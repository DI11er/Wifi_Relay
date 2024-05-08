from app import app, db

from app.models import User, Device

from flask import redirect, url_for, request, flash

from flask_login import current_user, logout_user

from flask_admin.contrib.sqla import ModelView

from flask_admin import BaseView, expose, AdminIndexView, Admin


class MixinAdmin:
    def is_accessible(self):
        return current_user.is_authenticated and current_user.is_admin
    
    def inaccessible_callback(self, name, **kwargs):
        # redirect to login page if user doesn't have access
        return redirect(url_for('login', next=request.full_path))


class MyAdminIndexView(MixinAdmin, AdminIndexView):
    pass
    


class BaseModelView(MixinAdmin, ModelView):
    create_modal = True
    edit_modal = True
    # can_view_details = True


class UserAdminView(BaseModelView):
    column_labels = {
        'username': 'Имя пользователя',
        'email': 'Адрес электронной почты',
        'created_on': 'Время и дата создания',
        'updated_on': 'Время и дата релактирования',
        'password_hash': 'Пароль',
        'is_admin': 'Статуc администратора'
    }

    column_list = ('username', 'email', 'created_on', 'updated_on', 'is_admin')
    column_searchable_list = ('username', 'email')
    column_sortable_list = ('username', 'created_on', 'updated_on')
    column_filters = ('created_on', 'updated_on', 'is_admin')
    form_create_rules = ('username', 'email', 'password_hash', 'is_admin')
    form_excluded_columns = ('created_on', 'updated_on')
    form_edit_rules = ('username', 'email', 'is_admin')
    
    def on_model_change(self, form, model, is_created):
        try:
            model.set_password(form.password_hash.data)
        except:
            pass
        return super().on_model_change(form, model, is_created)
    
    
class DeviceAdminView(BaseModelView):
    column_labels = {
        'name':'Название устройства', 
        'note': 'Заметка', 
        'position_index': 'Позиционный индекс'
    }
    column_list = ('name', 'ip', 'note', 'position_index')
    column_searchable_list = ('name', 'ip')
    column_sortable_list = ('name', 'ip', 'position_index')
    form_edit_rules = ('name', 'ip', 'note', 'position_index')
    can_export = True


class HomeView(BaseView):
    @expose('/')
    def index(self):
        return redirect(url_for('index'))

    
class LogoutView(BaseView):
    @expose('/')
    def index(self):
        return redirect(url_for('logout'))


# class AnalyticsView(BaseView):
#     @expose('/')
#     def index(self):
#         return self.render('admin/analytics_index.html')



    
admin_manager = Admin(app, name='Server_relay', template_mode='bootstrap3', index_view=MyAdminIndexView())

admin_manager.add_view(HomeView(name='Главное меню'))  
admin_manager.add_view(UserAdminView(User, db.session, name='Пользователи'))
admin_manager.add_view(DeviceAdminView(Device, db.session, name='Устройства'))
admin_manager.add_view(LogoutView(name='Выход')) 
# admin_manager.add_view(AnalyticsView(name='Analytics', endpoint='analytics'))

