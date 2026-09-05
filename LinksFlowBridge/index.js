let socket = null;

function sendJson(value) {
  if (
    !socket ||
    socket.readyState !== WebSocket.OPEN
  ) {
    return;
  }

  socket.send(
    JSON.stringify(value)
  );
}


function handleMessage(message) {
  let request;

  try {
    request =
      JSON.parse(message);
  } catch (error) {

    sendJson({
      version: 1,
      success: false,
      error: "Invalid JSON"
    });

    return;
  }


  if (request.command === "ping") {

    sendJson({
      version: 1,
      id: request.id || "",
      success: true,
      result: "pong"
    });

    return;
  }


  sendJson({
    version: 1,
    id: request.id || "",
    success: false,
    error:
      "Unknown command: " +
      String(request.command || "")
  });
}


function connectToLinksFlow() {
  if (
    socket &&
    (
      socket.readyState === WebSocket.OPEN ||
      socket.readyState === WebSocket.CONNECTING
    )
  ) {
    return;
  }


  socket =
    new WebSocket(
      "ws://127.0.0.1:17321"
    );


  socket.onopen = () => {
    console.log(
      "LinksFlow Bridge conectado"
    );

    sendJson({
      version: 1,
      event: "bridgeReady"
    });
  };


  socket.onmessage = (event) => {
    console.log(
      "LinksFlow recibió:",
      event.data
    );

    handleMessage(
      event.data
    );
  };


  socket.onerror = (event) => {
    console.log(
      "LinksFlow Bridge error",
      event
    );
  };


  socket.onclose = () => {
    console.log(
      "LinksFlow Bridge desconectado"
    );

    socket = null;
  };
}


module.exports = {
  commands: {
    connectToLinksFlow
  }
};
