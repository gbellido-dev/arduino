var vpiedra = 0;
var vpapel = 1;
var vtijera = 2;
var vlagarto = 3;
var vspock = 4;
var rhombre = 0;
var rmaquina = 0;
var yaPulsado = 0;
var numeroJugador = 0;



  function PiedraFunction() {
	  if((yaPulsado == 1) || (numeroJugador == 0)) {return;}
       document.getElementById("usuario").innerHTML = "<img src='img/piedra.png'/>";
       document.getElementById("vs").innerHTML = "<img src='img/vs.png'/>";
	   sendData(vpiedra);
  }

  function PapelFunction() {
	  if((yaPulsado == 1) || (numeroJugador == 0)) {return;}
       document.getElementById("usuario").innerHTML = "<img src='img/papel.png'/>";
       document.getElementById("vs").innerHTML = "<img src='img/vs.png'/>";
	   sendData(vpapel);
  }
    
  function TijeraFunction() {
	  if((yaPulsado == 1) || (numeroJugador == 0)) {return;}
       document.getElementById("usuario").innerHTML = "<img src='img/tijera.png'/>";
       document.getElementById("vs").innerHTML = "<img src='img/vs.png'/>";
	   sendData(vtijera);
  }
  
  function LagartoFunction() {
	  if((yaPulsado == 1) || (numeroJugador == 0)) {return;}
       document.getElementById("usuario").innerHTML = "<img src='img/lagarto.png'/>";
       document.getElementById("vs").innerHTML = "<img src='img/vs.png'/>"; 
	   sendData(vlagarto);
  }
  
  function SpockFunction() {
	  if((yaPulsado == 1) || (numeroJugador == 0)) {return;}
       document.getElementById("usuario").innerHTML = "<img src='img/spock.png'/>";
       document.getElementById("vs").innerHTML = "<img src='img/vs.png'/>";  
	   sendData(vspock);
  }
  
  function ResetFunction() {
	  yaPulsado = 0;
	  document.getElementById("usuario").innerHTML = "";
      document.getElementById("vs").innerHTML = "";
	  document.getElementById("maquina").innerHTML = "";
  }

  
  function sendData(objectId) {
	  var xhttp = new XMLHttpRequest();
	  yaPulsado = 1;
	  xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  
		}
	  };
	  xhttp.open("GET", "marcarSeleccion?objectId="+objectId, true);
	  xhttp.send();
  }
 
   function sendReset() {
	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  ResetFunction();
		}
	  };
	  xhttp.open("GET", "reset", true);
	  xhttp.send();
  }
  
  function sendFinReset() {
	  var xhttp = new XMLHttpRequest();
	  xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  ResetFunction();
		}
	  };
	  xhttp.open("GET", "finReset", true);
	  xhttp.send();
  }
  
  


function getStatus() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
		var str = this.responseText + "";
	/*reset-valorContrario-ganador
	  reset: 0- no hacer nada
			 1, 2- probocar reset
	  valorContrario: 0,1,2,3,4 seleccionar imagen
					  5 no hay movimiento contrario
	  ganador: 0- empate
			   1- gana jugador 1
			   2- gana jugador 2
			   3- no hay movimiento contrario
	*/
		var statusData = str.split("-"); 
		//document.getElementById("rhombre").innerHTML = statusData;

		var resetValue = statusData[0];
		var valorContrario = statusData[1];
		var ganador = statusData[2];

		if(resetValue == numeroJugador){
			sendFinReset();
		}
	  
		if (yaPulsado == 1 && valorContrario == 0) {
			document.getElementById("oponente").innerHTML = "<img src='img/piedra.png'/>";

		}else if (yaPulsado == 1 && valorContrario == 1) {
			document.getElementById("oponente").innerHTML = "<img src='img/papel.png'/>";

		}else if (yaPulsado == 1 && valorContrario == 2) {
			document.getElementById("oponente").innerHTML = "<img src='img/tijera.png'/>";

		}else if (yaPulsado == 1 && valorContrario == 3) {
			document.getElementById("oponente").innerHTML = "<img src='img/lagarto.png'/>";

		}else if (yaPulsado == 1 && valorContrario == 4) {
			document.getElementById("oponente").innerHTML = "<img src='img/spock.png'/>";
		}
		  
		if (ganador == numeroJugador) {
			document.getElementById("usuario").innerHTML = "";
			document.getElementById("vs").innerHTML = "<img src='img/ganaste.png'/><br/><i class='icon-thumbsup'></i> Genial, ganaste!!!!<br/><br/><br/>";
			document.getElementById("oponente").innerHTML = "";
		} else if (ganador == 0) {
			document.getElementById("usuario").innerHTML = "";
			document.getElementById("vs").innerHTML = "<img src='img/empate.png'/><br/><i class='icon-thumbsdown'></i> Que coña!! Empate!<br/><br/><br/>";
			document.getElementById("oponente").innerHTML = "";
		} else if(ganador == 3){
			//Nada
		}else {
			document.getElementById("usuario").innerHTML = "";
			document.getElementById("vs").innerHTML = "<img src='img/perdiste.png'/><br/><i class='icon-thumbsdown'></i> Hoo no!! Perdiste!<br/><br/><br/>";
			document.getElementById("oponente").innerHTML = "";
		}
    }
  };
  xhttp.open("GET", "obtenerEstado", true);
  xhttp.send();
}

function getPlayerId() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
	  numeroJugador = this.responseText;
      document.getElementById("rhombre").innerHTML = "Player " + numeroJugador;
	  
	  setInterval(function() {
		  // Call a function repetatively with 2 Second interval
		  getStatus();
	  }, 2000); //2000mSeconds update rate
    }
  };
  xhttp.open("GET", "obtenerJugador", true);
  xhttp.send();
}
