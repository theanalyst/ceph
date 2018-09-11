// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */

#include "rgw_dmclock_scheduler.h"
#include "rgw_common.h" // for rgw PerfCounter
namespace rgw::dmclock {

AsyncScheduler::~AsyncScheduler()
{
  cancel();
  if (observer) {
    cct->_conf.remove_observer(this);
  }
}

const char** AsyncScheduler::get_tracked_conf_keys() const
{
  if (observer) {
    return observer->get_tracked_conf_keys();
  }
  static const char* keys[] = { "rgw_max_concurrent_requests", nullptr };
  return keys;
}

void AsyncScheduler::handle_conf_change(const ConfigProxy& conf,
					const std::set<std::string>& changed)
{
  if (observer) {
    observer->handle_conf_change(conf, changed);
  }
  if (changed.count("rgw_max_concurrent_requests")) {
    auto new_max = conf.get_val<int64_t>("rgw_max_concurrent_requests");
    max_requests = new_max > 0 ? new_max : std::numeric_limits<int64_t>::max();
  }
  queue.update_client_infos();
  schedule(crimson::dmclock::TimeZero);
}

struct ClientSum {
  uint64_t count{0};
  Cost cost{0};
};
constexpr auto client_count = static_cast<size_t>(client_id::count);
using ClientSums = std::array<ClientSum, client_count>;

void inc(ClientSums& sums, client_id client, Cost cost)
{
  auto& sum = sums[static_cast<size_t>(client)];
  sum.count++;
  sum.cost += cost;
}

void on_cancel(PerfCounters *c, const ClientSum& sum);
void on_process(PerfCounters* c, const ClientSum& rsum, const ClientSum& psum);

void AsyncScheduler::request_complete()
{
  --outstanding_requests;
  schedule(crimson::dmclock::TimeZero);
}

void AsyncScheduler::cancel()
{
  ClientSums sums;

  queue.remove_by_req_filter([&] (RequestRef&& request) {
      inc(sums, request->client, request->cost);
      auto c = static_cast<Completion*>(request.release());
      Completion::dispatch(std::unique_ptr<Completion>{c},
                           boost::asio::error::operation_aborted,
                           PhaseType::priority);
      return true;
    });
  timer.cancel();

  for (size_t i = 0; i < client_count; i++) {
    if (auto c = counters(static_cast<client_id>(i))) {
      on_cancel(c, sums[i]);
    }
  }
}

void AsyncScheduler::cancel(const client_id& client)
{
  ClientSum sum;

  queue.remove_by_client(client, false, [&] (RequestRef&& request) {
      sum.count++;
      sum.cost += request->cost;
      auto c = static_cast<Completion*>(request.release());
      Completion::dispatch(std::unique_ptr<Completion>{c},
                           boost::asio::error::operation_aborted,
                           PhaseType::priority);
    });
  if (auto c = counters(client)) {
    on_cancel(c, sum);
  }
  schedule(crimson::dmclock::TimeZero);
}

void AsyncScheduler::schedule(const Time& time)
{
  timer.expires_at(Clock::from_double(time));
  timer.async_wait([this] (boost::system::error_code ec) {
      // process requests unless the wait was canceled. note that a canceled
      // wait may execute after this AsyncScheduler destructs
      if (ec != boost::asio::error::operation_aborted) {
        process(get_time());
      }
    });
}

void AsyncScheduler::process(const Time& now)
{
  // must run in the executor. we should only invoke completion handlers if the
  // executor is running
  assert(get_executor().running_in_this_thread());

  ClientSums rsums, psums;

  while (outstanding_requests < max_requests) {
    auto pull = queue.pull_request(now);

    if (pull.is_none()) {
      // no pending requests, cancel the timer
      timer.cancel();
      break;
    }
    if (pull.is_future()) {
      // update the timer based on the future time
      schedule(pull.getTime());
      break;
    }
    ++outstanding_requests;

    // complete the request
    auto& r = pull.get_retn();
    auto client = r.client;
    auto phase = r.phase;
    auto started = r.request->started;
    auto cost = r.request->cost;
    auto c = static_cast<Completion*>(r.request.release());
    Completion::post(std::unique_ptr<Completion>{c},
                     boost::system::error_code{}, phase);

    if (auto c = counters(client)) {
      auto lat = Clock::from_double(now) - Clock::from_double(started);
      if (phase == PhaseType::reservation) {
        inc(rsums, client, cost);
        c->tinc(queue_counters::l_res_latency, lat);
      } else {
        inc(psums, client, cost);
        c->tinc(queue_counters::l_prio_latency, lat);
      }
    }
  }

  if (outstanding_requests >= max_requests) {
    if(perfcounter){
      perfcounter->inc(l_rgw_throttle);
    }
  }

  for (size_t i = 0; i < client_count; i++) {
    if (auto c = counters(static_cast<client_id>(i))) {
      on_process(c, rsums[i], psums[i]);
    }
  }
}

int SyncScheduler::add_request(const client_id& client, const ReqParams& params,
                               const Time& time, Cost cost)
{
  std::mutex req_mtx;
  std::condition_variable req_cv;
  ReqState rstate {ReqState::Wait};
  auto req = SyncRequest{client, time, cost, req_mtx, req_cv, rstate, counters};
  int r = queue.add_request_time(req, client, params, time, cost);
  if (r == 0) {
    if (auto c = counters(client)) {
      c->inc(queue_counters::l_qlen);
      c->inc(queue_counters::l_cost, cost);
    }
    queue.request_completed();
    // Perform a blocking wait until the request callback is called
    if (std::unique_lock<std::mutex> lk(req_mtx); rstate != ReqState::Wait) {
      req_cv.wait(lk, [&rstate] {return rstate != ReqState::Wait;});
    }
    if (rstate == ReqState::Cancelled) {
      //FIXME: decide on error code for cancelled request
      r = -ECONNABORTED;
    }
  } else {
      // post the error code
    if (auto c = counters(client)) {
      c->inc(queue_counters::l_limit);
      c->inc(queue_counters::l_limit_cost, cost);
    }
  }
  return r;
}

void SyncScheduler::handle_request_cb(const client_id &c,
				      std::unique_ptr<SyncRequest> req,
				      PhaseType phase, Cost cost)
{
  { std::lock_guard<std::mutex> lg(req->req_mtx);
    req->req_state = ReqState::Ready;
    req->req_cv.notify_one();
  }

  if (auto ctr = req->counters(c)) {
    auto lat = Clock::from_double(get_time()) - Clock::from_double(req->started);
    if (phase == PhaseType::reservation){
      ctr->tinc(queue_counters::l_res_latency, lat);
      ctr->inc(queue_counters::l_res);
      if (cost) ctr->inc(queue_counters::l_res_cost);
    } else if (phase == PhaseType::priority){
      ctr->tinc(queue_counters::l_prio_latency, lat);
      ctr->inc(queue_counters::l_prio);
      if (cost) ctr->inc(queue_counters::l_prio_cost);
    }
    ctr->dec(queue_counters::l_qlen);
    if (cost) ctr->dec(queue_counters::l_cost);
  }
}


void SyncScheduler::cancel(const client_id& client)
{
  ClientSum sum;

  queue.remove_by_client(client, false, [&](RequestRef&& request)
    {
      sum.count++;
      sum.cost += request->cost;
      {
	std::lock_guard <std::mutex> lg(request->req_mtx);
	request->req_state = ReqState::Cancelled;
	request->req_cv.notify_one();
      }
    });
  if (auto c = counters(client)) {
    on_cancel(c, sum);
  }

  queue.request_completed();
}

void SyncScheduler::cancel()
{
  ClientSums sums;

  queue.remove_by_req_filter([&](RequestRef&& request) -> bool
			     {
			       inc(sums, request->client, request->cost);
			       {
				 std::lock_guard<std::mutex> lg(request->req_mtx);
				 request->req_state = ReqState::Cancelled;
				 request->req_cv.notify_one();
			       }
			       return true;
			     });

  for (size_t i = 0; i < client_count; i++) {
    if (auto c = counters(static_cast<client_id>(i))) {
      on_cancel(c, sums[i]);
    }
  }
}

SyncScheduler::~SyncScheduler()
{
  cancel();
}

void on_cancel(PerfCounters *c, const ClientSum& sum)
{
  if (sum.count) {
    c->dec(queue_counters::l_qlen, sum.count);
    c->inc(queue_counters::l_cancel, sum.count);
  }
  if (sum.cost) {
    c->dec(queue_counters::l_cost, sum.cost);
    c->inc(queue_counters::l_cancel_cost, sum.cost);
  }
}

void on_process(PerfCounters* c, const ClientSum& rsum, const ClientSum& psum)
{
  if (rsum.count) {
    c->inc(queue_counters::l_res, rsum.count);
  }
  if (rsum.cost) {
    c->inc(queue_counters::l_res_cost, rsum.cost);
  }
  if (psum.count) {
    c->inc(queue_counters::l_prio, psum.count);
  }
  if (psum.cost) {
    c->inc(queue_counters::l_prio_cost, psum.cost);
  }
  if (rsum.count + psum.count) {
    c->dec(queue_counters::l_qlen, rsum.count + psum.count);
  }
  if (rsum.cost + psum.cost) {
    c->dec(queue_counters::l_cost, rsum.cost + psum.cost);
  }
}


ClientCounters::ClientCounters(CephContext *cct)
{
  clients[static_cast<size_t>(client_id::admin)] =
      queue_counters::build(cct, "dmclock-admin");
  clients[static_cast<size_t>(client_id::auth)] =
      queue_counters::build(cct, "dmclock-auth");
  clients[static_cast<size_t>(client_id::data)] =
      queue_counters::build(cct, "dmclock-data");
  clients[static_cast<size_t>(client_id::metadata)] =
      queue_counters::build(cct, "dmclock-metadata");
}

namespace queue_counters {

PerfCountersRef build(CephContext *cct, const std::string& name)
{
  if (!cct->_conf->throttler_perf_counter) {
    return {};
  }

  PerfCountersBuilder b(cct, name, l_first, l_last);
  b.add_u64(l_qlen, "qlen", "Queue size");
  b.add_u64(l_cost, "cost", "Cost of queued requests");
  b.add_u64_counter(l_res, "res", "Requests satisfied by reservation");
  b.add_u64_counter(l_res_cost, "res_cost", "Cost satisfied by reservation");
  b.add_u64_counter(l_prio, "prio", "Requests satisfied by priority");
  b.add_u64_counter(l_prio_cost, "prio_cost", "Cost satisfied by priority");
  b.add_u64_counter(l_limit, "limit", "Requests rejected by limit");
  b.add_u64_counter(l_limit_cost, "limit_cost", "Cost rejected by limit");
  b.add_u64_counter(l_cancel, "cancel", "Cancels");
  b.add_u64_counter(l_cancel_cost, "cancel_cost", "Canceled cost");
  b.add_time_avg(l_res_latency, "res latency", "Reservation latency");
  b.add_time_avg(l_prio_latency, "prio latency", "Priority latency");

  auto logger = PerfCountersRef{ b.create_perf_counters(), cct };
  cct->get_perfcounters_collection()->add(logger.get());
  return logger;
}

} // namespace queue_counters

} // namespace rgw::dmclock
