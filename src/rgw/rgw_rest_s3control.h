
// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2019 SUSE LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */

#pragma once
#include "rgw_rest.h"
#include "rgw_rest_s3.h"

class RGWHandler_S3AccountPublicAccessBlock : public RGWHandler_REST_S3 {
protected:
  // RGWOp *op_get() override;
  // RGWOp *op_put() override;
  // RGWOp *op_delete() override;
public:
  using RGWHandler_REST_S3::RGWHandler_REST_S3;
  ~RGWHandler_S3AccountPublicAccessBlock() override = default;
};

class RGWRESTMgr_S3AccountPublicAccessBlock : public RGWRESTMgr {
public:
  RGWRESTMgr_S3AccountPublicAccessBlock() = default;
  ~RGWRESTMgr_S3AccountPublicAccessBlock() override = default;

  RGWHandler_REST* get_handler(struct req_state* const s,
                               const rgw::auth::StrategyRegistry& auth_registry,
                               const std::string&) override {
    return new RGWHandler_S3AccountPublicAccessBlock(auth_registry);
  }
};

class RGWRESTMgr_S3Control : public RGWRESTMgr {
public:
  ~RGWRESTMgr_S3Control() override = default;
  RGWRESTMgr_S3Control() {
    register_resource("PublicAccessBlock",
                      new RGWRESTMgr_S3AccountPublicAccessBlock());
  }
};

