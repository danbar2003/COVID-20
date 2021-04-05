from http.server import BaseHTTPRequestHandler, HTTPServer
import logging


def get_file(file_path='index.html'):
    with open(file_path, 'rb') as file:
        return file.read()


class S(BaseHTTPRequestHandler):
    def _set_response(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        print(self.path)
        self._set_response()
        data = b''
        if self.path == '/':
            data += get_file()
        else:
            data += get_file(self.path[1:])
        self.wfile.write(data)  # ty 

def run(server_class=HTTPServer, handler_class=S, port=80):
    logging.basicConfig(level=logging.INFO)
    server_address = ('127.0.0.1', port)
    httpd = server_class(server_address, handler_class)
    logging.info('Starting httpd...\n')
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    logging.info('Stopping httpd...\n')


if __name__ == '__main__':
    run()
