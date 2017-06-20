#!/usr/bin/env python
import multiprocessing
import tempfile

import boto
import boto.s3.connection
import sh

from multiprocessing import Pool, TimeoutError, cpu_count
from functools import partial
class RGWConn(object):
    def __init__(self, access_key, secret, host, port, is_secure=False):
        self.conn = boto.connect_s3(
            aws_access_key_id = access_key,
            aws_secret_access_key = secret,
            host = host,
            port = port,
            is_secure = is_secure,
            calling_format = boto.s3.connection.OrdinaryCallingFormat(),
        )

    # @property
    # def bucket(self):
    #     return self.bucket

    # @bucket.setter
    def create_bucket(self, bucket_name):
        self.bucket = self.conn.create_bucket(bucket_name)

    def teardown(self):
        for b in self.conn.get_all_buckets():
            self.nuke_bucket(b)

    def nuke_bucket(self, bucket=None):
        bucket = bucket or self.bucket
        # TODO: fall back to normal delete if multidelete fails
        keys = bucket.get_all_keys()
        r = bucket.delete_keys(keys)
        if len(r.errors) == 0:
            self.conn.delete_bucket(bucket)

    def upload_keys(self,count,size):
        for i in range(count):
            key_name = 'key' + str(i)
            self.upload_key(key_name, size)

    def do_single_upload(self, key_name, size):
        print 'uploading stuff'
        with tempfile.TemporaryFile() as f:
            f.seek(size - 1)
            f.write(b'0')
            k = self.bucket.new_key(key_name)
            ret = k.set_contents_from_file(f,rewind=True)
        return ret

    def _upload_part(self, mp, size, part_num):
        with tempfile.TemporaryFile() as f:
            print 'doing stuff'
            f.seek(size - 1)
            f.write(b'0')
            f.seek(0)
            mp.upload_part_from_file(f, part_num)

    def multipart_upload_key(self, key_name, size, chunksize=50*1024*1024):
        chunkcount = size/chunksize
        last_chunk = size%chunksize
        mp = self.bucket.initiate_multipart_upload(key_name)
        pool = Pool(processes=8)
        [pool.apply_async(partial(self._upload_part, mp, chunksize),i) for i in range(1,chunkcount)]
        for i in range(1,chunkcount):
            self._upload_part(mp, chunksize, i)
        # pool.imap(partial(self._upload_part, mp, chunksize), range(1,chunkcount))
        if last_chunk != 0:
            self._upload_part(mp, last_chunk, chunkcount+1)

        mp.complete_upload()

    def upload_key(self, key_name, size):
        if size > 50*1024**2:
            self.multipart_upload_key(key_name, size)
        else:
            self.do_single_upload(key_name, size)

    def delete_key(self, key_name, bucket=None):
        bucket = bucket or self.bucket
        bucket.delete_key(key_name)


class RGWAdmin(object):
    def __init__(self, _uid):
        self.rgw_admin = sh.Command('radosgw-admin')
        self.uid = _uid

    def create_user(self, access_key, secret, uid):
        return self.rgw_admin('user','create','--uid', uid,
                              '--display-name', uid,
                              '--access-key', access_key,
                              '--secret', secret)

    def teardown(self):
        return self.rgw_admin('user', 'rm', '--uid', self.uid, '--purge-data')

    def bucket_quota_set_size(self, bucket_name, size):
        return self.rgw_admin('quota','set',
                              '--bucket', bucket_name,
                              '--max-size', size)

# for the lack of a better name
class RGWCtx(object):
    def __init__(self, uid, access_key, secret, host, port):
        self.uid = uid
        self.rgw_admin = RGWAdmin(self.uid)
        self.rgw_admin.create_user(access_key, secret, self.uid)
        self.conn = RGWConn(access_key, secret,host, port)

    def teardown(self):
        self.conn.teardown()
        self.rgw_admin.teardown()
