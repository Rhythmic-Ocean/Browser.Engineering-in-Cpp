function Node(handle) {
  this.handle = handle;
}

console = {
  log(a) {
    log(a);
  }
}

document = {
  querySelectorAll(s) {
    var handles = querySelectorAll(s);
    return handles.map(function (h) { return new Node(h) });
  }
}
