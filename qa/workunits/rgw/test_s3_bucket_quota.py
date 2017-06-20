#!/usr/bin/env python
import os
from collections import namedtuple

from boto.exception import S3ResponseError
import pytest
import sh

from s3utils import RGWAdmin, RGWConn, RGWCtx
from contextlib import contextmanager
ctr = 0

Credentials = namedtuple('Credentials', 'uid access_key secret')

@contextmanager
def does_not_raise(exc, msg):
    try:
        yield
    except exc:
        raise pytest.fail('%s: test raised %s'  % (msg, exc))

def _make_credentials(c):
    uid = 'qatest'+str(ctr)
    access_key = 'access' + str(ctr)
    secret = 'secret' + str(ctr)
    return Credentials(uid, access_key, secret)

@pytest.fixture
def rgw_ctx():
    global ctr
    creds = _make_credentials(ctr)
    host = os.getenv('RGW_HOST') or 'localhost'
    port = int(os.getenv('RGW_PORT')) or 8000
    ctx = RGWCtx(creds.uid, creds.access_key, creds.secret, host, port)
    print 'user created'
    bucket_name = creds.uid + 'bucket' + str(ctr)
    ctx.conn.create_bucket(bucket_name)
    yield ctx
    ctx.teardown()
    ctr += 1

def test_quota_size(rgw_ctx):
    rgw_conn = rgw_ctx.conn
    rgw_admin = rgw_ctx.rgw_admin
    bucket = rgw_conn.bucket.name
    mysize = 2*100*1024**2
    with does_not_raise(S3ResponseError, 'upload object failed'):
        rgw_conn.upload_key('key1', mysize)

    quota_size = 100*1024**2
    ret = rgw_admin.bucket_quota_set_size(bucket, quota_size)
    assert ret.exit_code == 0

    with does_not_raise(S3ResponseError, 'delete object after setting quota failed'):
        rgw_conn.delete_key('key1')

    with does_not_raise(S3ResponseError, 'set max size and ensure that objects beyond max size is allowed failed'):
        rgw_conn.upload_key('key2',quota_size)
