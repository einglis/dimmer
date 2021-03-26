#!/usr/bin/env python3

from http.server import BaseHTTPRequestHandler, HTTPServer
from http.client import parse_headers
from jinja2 import Environment, FileSystemLoader
from socketserver import ThreadingMixIn
from threading import Event
from time import sleep
from urllib.parse import parse_qs, urlparse, urlsplit
import json
import time
import random # just for testing
import re
import uuid

from fish4 import PeripheralThread
from fish4 import dimmer, rako

hostName = "10.23.1.111"#"localhost"
serverPort = 8080

template="pie"


class Scene:
    def __init__(self):
        self.uuid = uuid.uuid4()
        self.name = str(self.uuid)
        self.levels = [ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ]

    def toJSON(self):
        return { "Scene": { 'uuid': str(self.uuid), 'name': self.name, 'levels': self.levels } }

    def fromJSON( d: dict ):
        if 'Scene' in d:

            s = Scene()
            return s
        else:
            return None

    def __str__(self):
        return f"Scene: {self.uuid}, '{self.name}' - {self.levels}"

zero_uuid = uuid.UUID(int=0)

live_scene = Scene()
live_scene.uuid = zero_uuid
live_scene.name = ""
live_scene.levels = [ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ]
all_scenes = [ ]

buttons = [ None ] * 4

button_event = Event()

def find_scene_by_uuid( uuid: str ) -> Scene:
    if uuid == "0" or uuid == str(zero_uuid):
        return live_scene

    try:
        return next(s for s in all_scenes if str(s.uuid) == uuid)
    except:
        return None

def pre_parse_args( args, decode ):
    args = parse_qs(args, keep_blank_values=0)

    if decode:
        temp_args = {}
        for key,values in args.items():
            str_values = []
            for v in values:
                str_values.append( v.decode('utf-8') )
            temp_args[ key.decode('utf-8')  ]= str_values
        args = temp_args

    for key,values in args.items():
        if len(values) == 1:
            if values[0].lower() == 'true':
                args[key] = True
            elif values[0].lower() == 'false':
                args[key] = False

    return args


def save_scenes():
    print("save scenes")

    def json_default(obj):
        try:
           return obj.toJSON()#__json__()
        except AttributeError:
           raise TypeError("{} can not be JSON encoded".format(type(obj)))

    config = { 'Scenes': all_scenes, 'Buttons': [ b for b in map(str, buttons) ] }

    with open("fish.json", "w") as fo:
        json.dump(config, fo, default=json_default, indent=4)


def load_scenes():
    print("load scenes")

    def json_obj_hook(d):
        print("obj_hook", d)
        if not isinstance(d, dict) or len(d) > 1:
            return d

        if 'Scene' in d:
            try:
                dv = d['Scene']
                s = Scene()
                s.uuid = uuid.UUID(dv['uuid'])
                s.name = dv['name']
                s.levels = [*dv['levels']]
                return s
            except:
                return d #lazy; we failed, just ignore it.
        else:
            return d

    try:
        with open("fish.json", "r") as fi:
            fish = json.load(fi, object_hook = json_obj_hook)
            print(fish)
            print(fish['Scenes'])
            print(fish['Buttons'])
            for f in fish['Scenes']:
                if isinstance(f, Scene):
                    all_scenes.append(f)
            for i,f in enumerate(fish['Buttons']):
                if i < len(buttons) and f:
                    buttons[i] = uuid.UUID(f)
    except:
        print("failed to load config")


def update_live():
    output = ''.join('%02x' % t for t in live_scene.levels)
    print("{%s}" % output)
    dimmer.tx(output)

def handle_up_down( delta ):
    for i, level in enumerate( live_scene.levels ):
        live_scene.levels[i] = max( 0, min( 255, level + delta ) )
    update_live()

    button_event.set()
    button_event.clear()

def handle_scene( button, surpress_event=False ):

    selected_scene = None

    if button==0:
        live_scene.levels = [ 0 ] * len(live_scene.levels)
        selected_scene = None

    elif button > 0 and button <= len(buttons):
        new_uuid = buttons[int(button)-1]
        print("new_uuid", new_uuid)
        new_scene = find_scene_by_uuid( str(new_uuid) ) # XXXEDD fudge
        print("new_scene", new_scene)
        if (new_scene):
            selected_scene = new_scene
            live_scene.levels = [*new_scene.levels]

    update_live()

    if not surpress_event:
        button_event.set()
        button_event.clear()

    return selected_scene





class MyServer(BaseHTTPRequestHandler):

    # def __init__(self, template):
    #     BaseHTTPRequestHandler.__init__(self)
    #     self.template = template

    def lazy_render_main_page(self, selected_scene=None):

        if not selected_scene:
            selected_scene = live_scene

        print("selected", selected_scene.uuid)


        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

        self.wfile.write(
            template.render(
                title="Fishsticks (Jinja2)",
                request_path=self.path,
                scripts=[],
                scenes = all_scenes,
                selected_scene=selected_scene,
                live_levels=live_scene.levels,
                buttons=buttons,
                update_disabled="disabled" if selected_scene == live_scene else ""
                ).encode()
            )


    def handle_save(self, args):
        print("SAVING GUFF %s" % (args))
        self.send_response(303) # see other
        self.send_header("Location", "../config.html")
        self.end_headers()

    def handle_unknown(self, path, selected_scene=None):
            self.send_response(303) # see other
            if not selected_scene:
                self.send_header("Location", "/")
            else:
                self.send_header("Location", f"/?uuid={str(selected_scene.uuid)}")
            self.end_headers()



    def do_POST(self):

        content_type = self.headers['content-type']
        if content_type == 'application/x-www-form-urlencoded' or content_type == 'text/plain;charset=UTF-8':
            content_length = int(self.headers['content-length'])
            # XXXEDD: catch no content case here.

            content = self.rfile.read(content_length)
            args = pre_parse_args( content, True ) # True == decode from bytes

            uuid = args['uuid'][0] if 'uuid' in args else None
            scene = find_scene_by_uuid( uuid )
            print(scene)

            path = urlsplit(self.path).path.lower()

            if path == "/slide":
                if 'slider' in args and 'value' in args:
                    val = int(args['value'][0])
                    slider = int(args['slider'][0])
                    val = min( max( val, 0 ), 255 )
                    if slider in range(1,7):
                        live_scene.levels[slider-1]=val

                update_live()
                self.send_response(204)
                self.end_headers()

            elif path == "/scene_configure":
                x = None
                if 'create_new' in args:
                    x = Scene()
                    x.levels = [*live_scene.levels]
                    all_scenes.append(x)
                elif 'update_existing' in args:
                    if scene:
                        new_levels = []
                        for x in range(6):
                            new_levels.append( int(args['range'+str(x+1)][0]) )
                        scene.levels = new_levels
                        scene.name = args['name'][0]
                    x = scene

                    save_scenes()

                elif 'delete_existing' in args:
                    if scene:
                        all_scenes.remove(scene)
                else:
                    print("CONFUSED!")
                self.handle_unknown(path,selected_scene=x) # lazy PRG pattern

            elif path == "/button_configure":
                for b in range(len(buttons)):
                    name = 'button'+str(b+1)+'_scene'
                    uuid = args[name][0] if name in args else None
                    buttons[b] = find_scene_by_uuid( uuid ).uuid
                    save_scenes()

                self.handle_unknown(path) # lazy PRG

            elif path == '/save':
                print("POST save")
                self.handle_save(args)

            elif path == '/control':
                print("CONTROL")
                button = args['button'][0] if 'button' in args else None
                print("button", button)

                if button == 'u' or button == 'U':
                    handle_up_down( 8 )
                elif button == 'd' or button == 'D':
                    handle_up_down( -8 )
                elif button == '0' or button == '1' or button == '2' or button == '3' or button == '4':
                    handle_scene( int(button) )

                self.send_response(204)
                self.end_headers()


            else:
                self.handle_unknown(path)





        else:
            print("WARNING: unrecognised POST content-type: '%s'" % (content_type))




    def do_GET(self):
#        print(self.headers)

        query = urlsplit(self.path).query
        args = pre_parse_args( urlsplit(self.path).query, False ) # False == no decode needed
        path = urlsplit(self.path).path.lower()
        print("path", path)

        button = args['button'][0] if 'button' in args else None
        print("button", button)

        uuid = args['uuid'][0] if 'uuid' in args else None
        scene = find_scene_by_uuid( uuid )
        print("scene", scene)

        if path == '/':

            selected_scene=scene

            if button and ( button=="off" or int(button) == 0):
                selected_scene = handle_scene(0, surpress_event=True)
                # selected_scene = None
                # live_scene.levels = [ 0 ] * len(live_scene.levels)
                # update_live()

            elif button and int(button) > 0 and int(button) <= len(buttons):
                selected_scene = handle_scene(int(button), surpress_event=True)
                # new_uuid = buttons[int(button)-1]
                # print("new_uuid", new_uuid)
                # new_scene = find_scene_by_uuid( str(new_uuid) ) # XXXEDD fudge
                # print("new_scene", new_scene)
                # if (new_scene):
                #     selected_scene = new_scene
                #     live_scene.levels = [*new_scene.levels]
                #     update_live()


            self.lazy_render_main_page(selected_scene)

        elif path == "/styles.css":
            self.send_response(200)
            self.send_header("Content-type", "text/css")
            self.end_headers()
            with open("static/styles.css", "rb") as f:
                self.wfile.write(f.read())
                f.close()
        elif path == "/static/webtest.js":
            self.send_response(200)
            self.send_header("Content-type", "text/js")
            self.end_headers()
            with open("static/webtest.js", "rb") as f:
                self.wfile.write(f.read())
                f.close()
        elif path == "/json/scene":
            temp = json.dumps(scene.toJSON()) if scene else ""

            self.send_response(200)
            self.send_header("Content-type", "text/json")
            self.end_headers()
            self.wfile.write(temp.encode())

        elif path == '/control':
            print("CONTROL")

        elif path == "/json/long_poll_buttons":
            fish = random.randint(1,1000)
            print("before sleep", fish);
            #sleep(1)
            button_event.wait()
            print("after sleep", fish);
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate");
            self.send_header("Pragma", "no-cache");
            self.send_header("Expires", "-1");
            self.end_headers()

            #self.wfile.write( bytes( f"{random.sample(range(0, 255), 6)}" , "utf-8" ) )
            self.wfile.write( bytes( f"{live_scene.levels}" , "utf-8" ) )

        elif path == "/json/status.txt":

            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate");
            self.send_header("Pragma", "no-cache");
            self.send_header("Expires", "-1");
            self.end_headers()

            self.wfile.write( bytes( \
"""
<p>Fish: <b>Yes</b></p>
<p>Chips: <b>Small</b></p>
<p>Gravy: <b>%s</b></p>
""" % random.randint(1,100)
, "utf-8" ) )
        elif path == '/save':
            print("GET save")
            self.handle_save(args)
        else:
            self.handle_unknown(path)


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""
    pass

if __name__ == "__main__":

    load_scenes()
    save_scenes()

    # for i in range(4):
    #     x = Scene()
    #     x.name = f"My scene {i}"
    #     x.levels = random.sample(range(0, 255), 6)
    #     all_scenes.append(x)

    PeripheralThread().start()
    rako.fns( scene=handle_scene, updown=handle_up_down )

    file_loader = FileSystemLoader("templates")
    env = Environment(loader=file_loader)
    template = env.get_template("fancy.html")

    webServer = ThreadedHTTPServer((hostName, serverPort), MyServer)
    print("Server started http://%s:%s" % (hostName, serverPort))

    webServer.daemon_threads = True

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")
