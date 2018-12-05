#!/usr/bin/env python3
"""
Very simple HTTP server in python for logging requests
Usage::
./server.py [<address>] [<port>]
"""

import deploy
from http.server import BaseHTTPRequestHandler, HTTPServer
import logging

class S(BaseHTTPRequestHandler):
  def set_response(self):
    self.send_response(200)
    self.send_header('Content-type', 'application/json')
    self.end_headers()

  def do_POST(self):
    content_length = int(self.headers['Content-Length'])
    post_data = self.rfile.read(content_length)

    logging.info(" POST request,\nPath: %s\nHeaders:\n%sBody:\n%s\n",
    str(self.path), str(self.headers), post_data.decode('utf-8'))

    responce = {}
    if (post_data['type'] == 'msig_v1'):
      responce = deploy.deploy_v1(post_data['account'])
    if (post_data['type'] == 'msig_v2'):
      responce = deploy.deploy_v2(post_data['account'])

    self._set_response()
    self.wfile.write(responce.encode())

    logging.info(" POST responce,\nPath: %s\nHeaders:\n%sBody:\n%s\n",
    str(self.path), str(self.headers), responce)

def run(server_class=HTTPServer, handler_class=S, address = 'localhost', port=8080):
  logging.basicConfig(level=logging.INFO)
  server_address = (address, port)
  httpd = server_class(server_address, handler_class)
  logging.info('\nStarting httpd service\nAddress: %s\nPort:%s\n', str(address), str(port))
  try:
    httpd.serve_forever()
  except KeyboardInterrupt:
    pass
    httpd.server_close()
    logging.info('Stopping httpd service\n')

if __name__ == '__main__':
  from sys import argv

  if len(argv) == 3:
    run(address=str(argv[1]), port=int(argv[2]))
  else:
    run()
