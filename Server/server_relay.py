from app import app, db, socketio

from config import ConfigAppRun

from app.models import User, Device



@app.shell_context_processor
def make_shell_context():
    return {'socketio': socketio, 'db': db, 'User': User, 'Device': Device}



if __name__ == '__main__':   
    socketio.run(
        app,
        host=ConfigAppRun.HOST,
        port=ConfigAppRun.PORT,
        use_reloader=ConfigAppRun.USE_RELOADER
    )