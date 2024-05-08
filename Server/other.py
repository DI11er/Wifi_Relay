# # этот декоратор выполняет функцию еще до обработки первого запроса
# @app.before_first_request
# def before_first_request():
#     print("before_first_request() called")

# # выполняет функцию до обработки запроса
# @app.before_request
# def before_request():
#     _log.debug("before_request() called")

# # выполняет функцию после обработки запроса. 
# # Такая функция не будет вызвана при возникновении исключений в обработчике запросов. 
# # Она должна принять объект ответа и вернуть тот же или новый ответ.
# @app.after_request
# def after_request(response):
#     _log.debug("after_request() called")
#     return response

# # этот декоратор похож на after_request. 
# # Но вызванная функция всегда будет выполняться вне зависимости от того, возвращает ли обработчик исключение или нет.
# @app.teardown_request
# def teardown_request(response):
#     _log.debug("teardown_request() called")
#     return response