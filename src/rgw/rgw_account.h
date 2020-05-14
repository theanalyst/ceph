// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2020 SUSE LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */

#pragma once

struct AccountQuota {
  uint16_t max_users {1000};
  uint16_t max_roles {1000};
}

class RGWAccount {
  std::string id;
  std::string tenant;
  AccountQuota account_quota;
public:
  RGWAccount(std::string&& _id) : id(std::move(_id)) {}
  RGWAccount(const std::string& _id): id(_id) {}

  RGWAccount(std::string&& _id,
	     std::string&& _tenant) : id(std::move(_id)),
				      tenant(std::move(_tenant))
  {}

  ~RGWAccount() = default;

  void encode(bufferlist& bl) const;
  void decode(bufferlist::const_iterator& bl);
};

class RGWAccountCtl
{
public:
  int add_user(const rgw_user& user);
  int add_role(const RGWRole& role);

  int list_users();

  int remove_user(const rgw_user& user);
  int remove_role(const RGWRole& role);
}
