LISTENERS = {}
/*
 * HANDLE-1 : {
 *    ACTION-1: {
 *      CALL_BACK-1.... , 
 *      CALL_BACK-2.....
 *    },
 *    ACTION-2: {
 *      CALL_BACK-3.... , 
 *      CALL_BACK-4.....
 *    }, ...
 * }
 * HANDLE-2: {
 *    ACTION-3: {
 *      CALL_BACK-5.... , 
 *      CALL_BACK-6.....
 *    },
 *    ACTION-4: {
 *      CALL_BACK-7.... , 
 *      CALL_BACK-8.....
 *    }, ...
 * }
 *
 * */


function Event(type) {
  this.type = type;
  this.do_default = true;
}

Event.prototype.preventDefault = function () {
  this.do_default = false;
}
function Node(handle) {
  this.handle = handle;
}

Node.prototype.getAttribute = function (attr) {
  return getAttribute(this.handle, attr);
}

Node.prototype.addEventListener = function (type, listener) {
  if (!LISTENERS[this.handle]) LISTENERS[this.handle] = {};
  var dict = LISTENERS[this.handle];
  if (!dict[type]) dict[type] = [];
  var list = dict[type];
  list.push(listener);
}

Node.prototype.dispatchEvent = function (evt) {
  var type = evt.type;
  var handle = this.handle;
  var list = (LISTENERS[handle] && LISTENERS[handle][type]) || [];
  for (var i = 0; i < list.length; i++) {
    list[i].call(this, evt);
  }
  return evt.do_default;
}

//Defined innerHTML such that when it's assigned to, the function under it runs
/*
 * eg: var node = new Node(4);
 *     node.innerHTML = "<p>Hello</p>"
 * */
Object.defineProperty(Node.prototype, 'innerHTML', {
  set: function (s) {
    //Duk/C API that runs when innerHTML's assgined to
    innerHTML_set(this.handle, s.toString());
  }
});

console = {
  log(a) {
    log(a);
  }
}

document = {
  querySelectorAll(s) {
    var handles = querySelectorAll(s);
    //Iterates over the ints from handles, convert it to Node and returns
    return handles.map(function (h) { return new Node(h) });
  }
}

