let socket = null;

function connectToLinksFlow() {
  console.log("LinksFlow Bridge: intentando conectar...");

  if (
    socket &&
    (
      socket.readyState === WebSocket.OPEN ||
      socket.readyState === WebSocket.CONNECTING
    )
  ) {
    console.log("LinksFlow Bridge: ya conectado o conectando.");
    return;
  }

  socket =
    new WebSocket(
      "ws://127.0.0.1:17321"
    );

  socket.onopen = () => {
    console.log(
      "LinksFlow Bridge: WebSocket conectado"
    );

    socket.send(
      "hello from InDesign"
    );
  };

  socket.onmessage = (event) => {
    console.log(
      "LinksFlow Bridge: mensaje recibido:",
      event.data
    );
  };

  socket.onerror = (event) => {
    console.log(
      "LinksFlow Bridge: error WebSocket",
      event
    );
  };

  socket.onclose = (event) => {
    console.log(
      "LinksFlow Bridge: WebSocket cerrado",
      event.code,
      event.reason
    );

    socket = null;
  };
}

module.exports = {
  commands: {
    connectToLinksFlow
  }
};
