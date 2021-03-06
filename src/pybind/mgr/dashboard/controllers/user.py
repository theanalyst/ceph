# -*- coding: utf-8 -*-
from __future__ import absolute_import

import cherrypy

from . import BaseController, ApiController, RESTController, Endpoint
from .. import mgr
from ..exceptions import DashboardException, UserAlreadyExists, \
    UserDoesNotExist
from ..security import Scope
from ..services.access_control import SYSTEM_ROLES
from ..services.auth import JwtManager


@ApiController('/user', Scope.USER)
class User(RESTController):
    @staticmethod
    def _user_to_dict(user):
        result = user.to_dict()
        del result['password']
        return result

    @staticmethod
    def _get_user_roles(roles):
        all_roles = dict(mgr.ACCESS_CTRL_DB.roles)
        all_roles.update(SYSTEM_ROLES)
        try:
            return [all_roles[rolename] for rolename in roles]
        except KeyError:
            raise DashboardException(msg='Role does not exist',
                                     code='role_does_not_exist',
                                     component='user')

    def list(self):
        users = mgr.ACCESS_CTRL_DB.users
        result = [User._user_to_dict(u) for _, u in users.items()]
        return result

    def get(self, username):
        try:
            user = mgr.ACCESS_CTRL_DB.get_user(username)
        except UserDoesNotExist:
            raise cherrypy.HTTPError(404)
        return User._user_to_dict(user)

    def create(self, username=None, password=None, name=None, email=None,
               roles=None, enabled=True):
        if not username:
            raise DashboardException(msg='Username is required',
                                     code='username_required',
                                     component='user')
        user_roles = None
        if roles:
            user_roles = User._get_user_roles(roles)
        try:
            user = mgr.ACCESS_CTRL_DB.create_user(username, password, name,
                                                  email, enabled)
        except UserAlreadyExists:
            raise DashboardException(msg='Username already exists',
                                     code='username_already_exists',
                                     component='user')
        if user_roles:
            user.set_roles(user_roles)
        mgr.ACCESS_CTRL_DB.save()
        return User._user_to_dict(user)

    def delete(self, username):
        session_username = JwtManager.get_username()
        if session_username == username:
            raise DashboardException(msg='Cannot delete current user',
                                     code='cannot_delete_current_user',
                                     component='user')
        try:
            mgr.ACCESS_CTRL_DB.delete_user(username)
        except UserDoesNotExist:
            raise cherrypy.HTTPError(404)
        mgr.ACCESS_CTRL_DB.save()

    def set(self, username, password=None, name=None, email=None, roles=None,
            enabled=None):
        if JwtManager.get_username() == username and enabled is False:
            raise DashboardException(msg='You are not allowed to disable your user',
                                     code='cannot_disable_current_user',
                                     component='user')

        try:
            user = mgr.ACCESS_CTRL_DB.get_user(username)
        except UserDoesNotExist:
            raise cherrypy.HTTPError(404)
        user_roles = []
        if roles:
            user_roles = User._get_user_roles(roles)
        if password:
            user.set_password(password)
        user.name = name
        user.email = email
        if enabled is not None:
            user.enabled = enabled
        user.set_roles(user_roles)
        mgr.ACCESS_CTRL_DB.save()
        return User._user_to_dict(user)


@ApiController('/user/{username}')
class UserChangePassword(BaseController):
    @Endpoint('POST')
    def change_password(self, username, old_password, new_password):
        session_username = JwtManager.get_username()
        if username != session_username:
            raise DashboardException(msg='Invalid user context',
                                     code='invalid_user_context',
                                     component='user')
        try:
            user = mgr.ACCESS_CTRL_DB.get_user(session_username)
        except UserDoesNotExist:
            raise cherrypy.HTTPError(404)
        if not user.compare_password(old_password):
            raise DashboardException(msg='Invalid old password',
                                     code='invalid_old_password',
                                     component='user')
        user.set_password(new_password)
        mgr.ACCESS_CTRL_DB.save()
