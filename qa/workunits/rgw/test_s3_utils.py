#!/usr/bin/env python
import tempfile

import boto
import boto.s3.connection

class RGWConn(object):
    def __init__(self, access_key, secret, host, is_secure=False):
        self.conn = boto.connect_s3(
            aws_access_key_id = access_key,
            aws_secret_access_key = secret,
            host = host,
            is_secure = is_secure,
            calling_format = boto.s3.connection.OrdinaryCallingFormat(),
        )
        self.bucket = None

    @property
    def bucket(self, bucket_name):
        self.bucket = conn.create_bucket(bucket_name)

    def nuke_bucket(self, bucket=None):
        if bucket is not None:
            b = self.conn.get_bucket(bucket)
        else:
            b = self.bucket
        # TODO: fall back to normal delete if multidelete fails
        keys = b.get_all_keys()
        r = b.delete_keys(keys)
        if len(r.errors) == 0:
            self.conn.delete_bucket(bucket)

    def upload_keys(self,count,size):
        for i in range(count):
            key_name = 'key' + str(i)
            self.upload_key(key_name, size)

    def upload_key(self, key_name, size):
        with tempfile.TemporaryFile() as f:
            f.seek(size*(1024**2) - 1)
            f.write(b'0')
            k = self.bucket.new_key(key_name)
            k.set_contents_from_file(f,rewind=True)
