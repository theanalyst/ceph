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

#include "svc_role.h"
#include "svc_meta_be.h"

class RGWSI_Role_RADOS: public RGWSI_Role
{
 public:
  struct Svc {
    RGWSI_Zone *zone{nullptr};
    RGWSI_Meta *meta{nullptr};
    RGWSI_MetaBackend *meta_be{nullptr};
    RGWSI_SysObj *sysobj{nullptr};
  } svc;

  RGWSI_Role_RADOS(CephContext *cct) : RGWSI_Role(cct) {}
  ~RGWSI_Role_RADOS() {}

  void init(RGWSI_Zone *_zone_svc,
	    RGWSI_Meta *_meta_svc,
	    RGWSI_MetaBackend *_meta_be_svc,
	    RGWSI_SysObj *_sysobj_svc);

  RGWSI_MetaBackend_Handler * get_be_handler() override;

  int store_info(RGWSI_MetaBackend::Context *ctx,
  		 const RGWRole& role,
  		 RGWObjVersionTracker * const objv_tracker,
  		 const real_time& pmtime,
  		 bool exclusive,
  		 map<std::string, bufferlist> * pattrs,
  		 optional_yield y) override;

  int store_name(RGWSI_MetaBackend::Context *ctx,
		 const std::string& role_id,
  		 const std::string& name,
		 const std::string& tenant,
  		 RGWObjVersionTracker * const objv_tracker,
  		 const real_time& mtime,
  		 bool exclusive,
  		 optional_yield y) override;

  int store_path(RGWSI_MetaBackend::Context *ctx,
		 const std::string& role_id,
		 const std::string& path,
		 const std::string& tenant,
  		 RGWObjVersionTracker * const objv_tracker,
  		 const real_time& mtime,
  		 bool exclusive,
  		 optional_yield y) override;

  int read_info(RGWSI_MetaBackend::Context *ctx,
		const std::string& role_id,
  		RGWRole *role,
  		RGWObjVersionTracker * const objv_tracker,
  		real_time * const pmtime,
  		map<std::string, bufferlist> * pattrs,
  		optional_yield y) override;

  int read_name(RGWSI_MetaBackend::Context *ctx,
		const std::string& name,
		const std::string& tenant,
		std::string& role_id,
  		RGWObjVersionTracker * const objv_tracker,
  		real_time * const pmtime,
  		optional_yield y) override;

  int read_path(RGWSI_MetaBackend::Context *ctx,
  		std::string& path,
  		RGWObjVersionTracker * const objv_tracker,
  		real_time * const pmtime,
  		optional_yield y) override;

  int delete_info(RGWSI_MetaBackend::Context *ctx,
		  const std::string& name,
		  RGWObjVersionTracker * const objv_tracker,
		  optional_yield y) override;


private:
  RGWSI_MetaBackend_Handler *be_handler;
  std::unique_ptr<RGWSI_MetaBackend::Module> be_module;
};
