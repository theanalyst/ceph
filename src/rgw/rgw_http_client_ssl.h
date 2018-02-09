// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2018 SUSE Linux GmBH
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef RGW_HTTP_CLIENT_SSL_H
#define RGW_HTTP_CLIENT_SSL_H

#include <mutex>
#include <vector>
#include "common/ceph_context.h"
#include "common/dout.h"
#include "rgw_common.h"

#define dout_context g_ceph_context
#define dout_subsys ceph_subsys_rgw

void rgw_ssl_locking_callback(int mode, int n, const char *file, int line);
unsigned long rgw_ssl_thread_id_callback();


class RGWSSLSetup
{
  std::vector <std::mutex> locks;
public:
 RGWSSLSetup(int n) : locks (n){}

  void set_lock(int id){
    try {
      locks.at(id).lock();
    } catch (std::out_of_range& e) {
      dout(0) << __func__ << "failed to set locks" << dendl;
    }
  }

  void clear_lock(int id){
    try {
      locks.at(id).unlock();
    } catch (std::out_of_range& e) {
      dout(0) << __func__ << "failed to unlock" << dendl;
    }
  }
};
#endif
