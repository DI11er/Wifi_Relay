import logging

from flask import render_template
from app import app, db, socketio

_log = logging.getLogger(__name__)


@app.errorhandler(404)
def not_found_error(error):
    _log.error('HTTP 404 Error Encountered')
    return render_template('404.html'), 404


@app.errorhandler(500)
def internal_error(error):
    db.session.rollback()
    _log.error('HTTP 500 Error Encountered')
    return render_template('500.html'), 500


# Обработчик ошибок
@socketio.on_error_default  # handles all namespaces without an explicit error handler
def default_error_handler(e):
    _log.error('An error has occurred:', str(e))