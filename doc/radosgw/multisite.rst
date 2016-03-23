=============
RGW Multisite
=============

.. versionadded:: Jewel

From Ceph release Jewel and beyond, you may configure each :term`Ceph Object
Gateway` to work in an active active zone configuration, allowing for writes to
non-master zones. Following are the basic terminologies that would be used:

- **Zone**: A zone is *logical* grouping of one or more Ceph Object Gateway
  instances. There will be one Zone that should be designated as the master zone
  in a zonegroup, which will handle all bucket and user creation.

- **Zonegroup**: A zonegroup consists of multiple zones, this approximately
  corresponds to what used to be called as a region in pre Jewel releases for
  federated deployments. There should be a master zonegroup that will handle
  changes to the system configuration.

- **Zonegroup map**: A zonegroup map is a configuration structure that holds the
  map of the entire system, ie. which zonegroup is the master, relationships
  between different zonegroups and certain configurables like storage policies.

- **Realm**: A realm is a container for zonegroups, this allows for separation
  of zonegroups themselves between clusters. It is possible to create multiple
  realm, making it easier to run completely different configurations in the same
  cluster.

- **Period**: A period holds the configuration structure for the current state
  of the realm, a period needs to be updated whenever there is a zone
  configuration change of the master zone.



Pool Configuration
==================

About this guide
================

In this guide we create a single zone group, with two seperate zones, which
actively sync data between them. There is no sync agent involved for mirroring
data changes between the RadosGWs and this allows for a much simpler
configuration scheme and active-active configurations. Please note that metadata
operations such as creating a user would still need to go through the master
zone, however data operations such as creation of buckets objects etc. can be
handled by any of the zones.

System Keys
===========

While configuring zones, RadosGW, expects creation of zone a system user, with
an S3 like access and secret keys. This allows another radosgw instance to pull
the configuration remotely, with the access and secret keys. For making
scripting and use of configuration management tools easier, it would be easier
to generate the access-key and secret-key before hand and use these. For the
purpose of the guide we assume an access key and secret key are set in the
environment. For eg., something like::

  $ SYSTEM_ACCESS_KEY=1555b35654ad1656d805
  $ SYSTEM_SECRET_KEY=h7GhxuBLTrlhVUyxSPUKUV8r/2EI4ngqJxD7iBdBYLhwluN30JaT3Q==

Naming conventions
==================

This section describes the process of setting up a master zone. For the purpose
of the guide, we'll assume a zonegroup called ``us`` spanning the United States,
which will be our master zonegroup. This will contain two zones written in a
``{zonegroup}-{zone}`` format, this is just a convention only and you are free
to choose a format you prefer to set these names. So in summary:

- Master Zonegroup: United States ``us``
- Master zone: United States, East Region: ``us-east``
- Secondary zone: United States, West Region: ``us-west``

This will be a part of a larger realm, say ``gold``

Creating a realm
----------------

Configure a realm called ``gold``, which for ::

  # radosgw-admin realm create --rgw-realm=gold
  {
    "id": "4a367026-bd8f-40ee-b486-8212482ddcd7",
    "name": "gold",
    "current_period": "09559832-67a4-4101-8b3f-10dfcd6b2707",
    "epoch": 1
  }


Note that every realm has an id, which allows for flexibility like renaming the
realm later, should the need arise. The ``current_period`` changes whenever we
change anything in the master zone. The ``epoch`` is incremented when there's a
change in configuration which doesn't modify the master zone

Deleting the default zonegroup
------------------------------

A simple installation of radosgw would assume the default zonegroup called
"default", since we no longer need this we begin by removing the default
zonegroup::

  # radosgw-admin zonegroup delete --rgw-zonegroup=default


Creating a master zonegroup
---------------------------

We'll be creating a zonegroup called ``us`` as a master zonegroup. A master
zonegroup will be in control of the zonegroup map and propagate changes to the
rest of the system. We will also set this zonegroup as the default, which allows
explicitly mentioning the ``rgw-zonegroup`` switch for later commands.

::

   # radosgw-admin zonegroup create --rgw-zonegroup=us --endpoints=http://rgw1:80 --master
   {
    "id": "d4018b8d-8c0d-4072-8919-608726fa369e",
    "name": "us",
    "api_name": "us",
    "is_master": "true",
    "endpoints": [
        "http:\/\/rgw1:80"
    ],
    "hostnames": [],
    "hostnames_s3website": [],
    "master_zone": "",
    "zones": [],
    "placement_targets": [],
    "default_placement": "",
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7"
    }

Now we make this as the default zonegroup via the following command ::

  # radosgw-admin zonegroup default --rgw-zonegroup=us


Creating a master zone
----------------------
Next we create a zone, and make it as the default zone. Note that for metadata
operations like user creation you would want to use this zone.

::

   #radosgw-admin zone create --rgw-zonegroup=us --rgw-zone=us-east --endpoints=http://rgw1:80 --access-key=$ZONE_ACCESS_KEY --secret=$ZONE_SECRET_KEY
   {
    "id": "83859a9a-9901-4f00-aa6d-285c777e10f0",
    "name": "us-east",
    "domain_root": "us-east.rgw.data.root",
    "control_pool": "us-east.rgw.control",
    "gc_pool": "us-east.rgw.gc",
    "log_pool": "us-east.rgw.log",
    "intent_log_pool": "us-east.rgw.intent-log",
    "usage_log_pool": "us-east.rgw.usage",
    "user_keys_pool": "us-east.rgw.users.keys",
    "user_email_pool": "us-east.rgw.users.email",
    "user_swift_pool": "us-east.rgw.users.swift",
    "user_uid_pool": "us-east.rgw.users.uid",
    "system_key": {
        "access_key": "1555b35654ad1656d804",
        "secret_key": "h7GhxuBLTrlhVUyxSPUKUV8r\/2EI4ngqJxD7iBdBYLhwluN30JaT3Q=="
    },
    "placement_pools": [
        {
            "key": "default-placement",
            "val": {
                "index_pool": "us-east.rgw.buckets.index",
                "data_pool": "us-east.rgw.buckets.data",
                "data_extra_pool": "us-east.rgw.buckets.non-ec",
                "index_type": 0
            }
        }
    ],
    "metadata_heap": "us-east.rgw.meta",
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7"
    }

   # radosgw-admin zone default --rgw-zone=us-east

Next we add the zone to the zonegroup::

  # radosgw-admin zonegroup add --rgw-zonegroup=us --rgw-zone=us-east


Creating system users
---------------------

Next we create the system users for accessing the zone pools, note that these
keys would be used when configuring the secondary zone::

  # radosgw-admin user create --uid=zone.user --display-name="Zone
  User" --access-key=$SYSTEM_ACCESS_KEY --secret=$SYSTEM_SECRET_KEY --system


Update the period
-----------------
Since we have now made a change in the master zone configuration, we need to
commit these zone changes to reflect in the realm configuration structure. This
is what the period would look like initially.

::
   # radosgw-admin period get
   {
    "id": "09559832-67a4-4101-8b3f-10dfcd6b2707",
    "epoch": 1,
    "predecessor_uuid": "",
    "sync_status": [],
    "period_map": {
        "id": "09559832-67a4-4101-8b3f-10dfcd6b2707",
        "zonegroups": [],
        "short_zone_ids": []
    },
    "master_zonegroup": "",
    "master_zone": "",
    "period_config": {
        "bucket_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        },
        "user_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        }
    },
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7",
    "realm_name": "gold",
    "realm_epoch": 1
    }

Now we update the period and commit the changes::

  # radosgw-admin period update --commit
  {
    "id": "b5e4d3ec-2a62-4746-b479-4b2bc14b27d1",
    "epoch": 1,
    "predecessor_uuid": "09559832-67a4-4101-8b3f-10dfcd6b2707",
    "sync_status": [ ""... # truncating the output here
    ],
    "period_map": {
        "id": "b5e4d3ec-2a62-4746-b479-4b2bc14b27d1",
        "zonegroups": [
            {
                "id": "d4018b8d-8c0d-4072-8919-608726fa369e",
                "name": "us",
                "api_name": "us",
                "is_master": "true",
                "endpoints": [
                    "http:\/\/rgw1:80"
                ],
                "hostnames": [],
                "hostnames_s3website": [],
                "master_zone": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                "zones": [
                    {
                        "id": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                        "name": "us-east",
                        "endpoints": [
                            "http:\/\/rgw1:80"
                        ],
                        "log_meta": "true",
                        "log_data": "false",
                        "bucket_index_max_shards": 0,
                        "read_only": "false"
                    }
                ],
                "placement_targets": [
                    {
                        "name": "default-placement",
                        "tags": []
                    }
                ],
                "default_placement": "default-placement",
                "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7"
            }
        ],
        "short_zone_ids": [
            {
                "key": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                "val": 630926044
            }
        ]
    },
    "master_zonegroup": "d4018b8d-8c0d-4072-8919-608726fa369e",
    "master_zone": "83859a9a-9901-4f00-aa6d-285c777e10f0",
    "period_config": {
        "bucket_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        },
        "user_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        }
    },
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7",
    "realm_name": "gold",
    "realm_epoch": 2
    }


Starting the radosgw
--------------------

Before starting the radosgw, the rgw zone and port options need to be mentioned
in the configuration file. For more details refer to the `Install Ceph Gateway`_
section of the guide. The configuration section for radosgw should resemble::

  [client.rgw.us-east-1]
  rgw_frontends="civetweb port=80"
  rgw_zone=us-east

And start the Ceph Object gateway (according to the OS installation) ::

  sudo systemctl start ceph-radosgw.service


Configuring the Secondary zone
==============================

The following steps will be performed on the node hosting the secondary zone.
You would've to delete the default zonegroup as mentioned for the primary zone.

Realm configuration
-------------------

Since a realm was already configured from the first gateway, we pull and make
that realm the default here::

  # radosgw-admin realm pull --url=http://rgw1:80
  --access-key=$SYSTEM_ACCESS_KEY --secret-key=$SYSTEM_SECRET_KEY
  {
    "id": "4a367026-bd8f-40ee-b486-8212482ddcd7",
    "name": "gold",
    "current_period": "b5e4d3ec-2a62-4746-b479-4b2bc14b27d1",
    "epoch": 2
  }

  # radosgw-admin realm default --rgw-realm=gold

Secondary Zone Configuration
----------------------------

We first set the default zonegroup to the created ``us`` zonegroup::

  # radosgw-admin zonegroup default --rgw-zonegroup=us

Next we create the new zone, ``us-west``, with the same system keys::

  # radosgw-admin zone create --rgw-zonegroup=us --rgw-zone=us-west
  --access-key=$ZONE_ACCESS_KEY --secret=$ZONE_SECRET_KEY --endpoints=http://rgw2:80
  {
    "id": "d9522067-cb7b-4129-8751-591e45815b16",
    "name": "us-west",
    "domain_root": "us-west.rgw.data.root",
    "control_pool": "us-west.rgw.control",
    "gc_pool": "us-west.rgw.gc",
    "log_pool": "us-west.rgw.log",
    "intent_log_pool": "us-west.rgw.intent-log",
    "usage_log_pool": "us-west.rgw.usage",
    "user_keys_pool": "us-west.rgw.users.keys",
    "user_email_pool": "us-west.rgw.users.email",
    "user_swift_pool": "us-west.rgw.users.swift",
    "user_uid_pool": "us-west.rgw.users.uid",
    "system_key": {
        "access_key": "1555b35654ad1656d804",
        "secret_key": "h7GhxuBLTrlhVUyxSPUKUV8r\/2EI4ngqJxD7iBdBYLhwluN30JaT3Q=="
    },
    "placement_pools": [
        {
            "key": "default-placement",
            "val": {
                "index_pool": "us-west.rgw.buckets.index",
                "data_pool": "us-west.rgw.buckets.data",
                "data_extra_pool": "us-west.rgw.buckets.non-ec",
                "index_type": 0
            }
        }
    ],
    "metadata_heap": "us-west.rgw.meta",
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7"
    }


Updating the period
-------------------
Now to propagate the zonegroup-map changes, we update and commit the period::

  # radosgw-admin period update --commit --rgw-zone=us-west
  {
    "id": "b5e4d3ec-2a62-4746-b479-4b2bc14b27d1",
    "epoch": 3,
    "predecessor_uuid": "09559832-67a4-4101-8b3f-10dfcd6b2707",
    "sync_status": [
        "", # truncated
    ],
    "period_map": {
        "id": "b5e4d3ec-2a62-4746-b479-4b2bc14b27d1",
        "zonegroups": [
            {
                "id": "d4018b8d-8c0d-4072-8919-608726fa369e",
                "name": "us",
                "api_name": "us",
                "is_master": "true",
                "endpoints": [
                    "http:\/\/rgw1:80"
                ],
                "hostnames": [],
                "hostnames_s3website": [],
                "master_zone": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                "zones": [
                    {
                        "id": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                        "name": "us-east",
                        "endpoints": [
                            "http:\/\/rgw1:80"
                        ],
                        "log_meta": "true",
                        "log_data": "true",
                        "bucket_index_max_shards": 0,
                        "read_only": "false"
                    },
                    {
                        "id": "d9522067-cb7b-4129-8751-591e45815b16",
                        "name": "us-west",
                        "endpoints": [
                            "http:\/\/rgw2:80"
                        ],
                        "log_meta": "false",
                        "log_data": "true",
                        "bucket_index_max_shards": 0,
                        "read_only": "false"
                    }
                ],
                "placement_targets": [
                    {
                        "name": "default-placement",
                        "tags": []
                    }
                ],
                "default_placement": "default-placement",
                "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7"
            }
        ],
        "short_zone_ids": [
            {
                "key": "83859a9a-9901-4f00-aa6d-285c777e10f0",
                "val": 630926044
            },
            {
                "key": "d9522067-cb7b-4129-8751-591e45815b16",
                "val": 329470157
            }
        ]
    },
    "master_zonegroup": "d4018b8d-8c0d-4072-8919-608726fa369e",
    "master_zone": "83859a9a-9901-4f00-aa6d-285c777e10f0",
    "period_config": {
        "bucket_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        },
        "user_quota": {
            "enabled": false,
            "max_size_kb": -1,
            "max_objects": -1
        }
    },
    "realm_id": "4a367026-bd8f-40ee-b486-8212482ddcd7",
    "realm_name": "gold",
    "realm_epoch": 2
    }

You can observe that the period epoch number has incremented, indicating a
change in the configuration

Starting the Ceph Object Gateway
--------------------------------

This is similar to starting the object gateway in the first zone, only
difference being in the ``rgw zone`` configurable.
