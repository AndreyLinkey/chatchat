# chatchat
Multi thread chat client server app

To run server execute command <b>chatchatd</b></br>
you can use following command line options for server:</br>
&ensp;<b>-p</b>  set server operating port</br>
&ensp;<b>-e</b>  enable echoing messages for incomming clients</br>
&ensp;<b>-m</b>  enable message processing for incomming clients</br>

To run client execute command <b>chatchat</b></br>
additional command line options also available:</br>
&ensp;<b>-a</b>  set server address</br>
&ensp;<b>-p</b>  set server operating port</br>
&ensp;<b>-g</b>  join given group</br>
&ensp;<b>-e</b>  enable echoing messages for client</br>
&ensp;<b>-m</b>  enable message processing for client</br>
To send message type it and press enter. All clients in the same group will receive message.
There are some special commands for client. Just type: </br>
&ensp;<b>#close</b></br>
&ensp;&ensp;close connection and terminate client</br>
&ensp;<b>#join <group_name></b></br>
&ensp;&ensp;join to given group, one client can join to several groups</br>
&ensp;<b>#leave <group_name></b></br>
&ensp;&ensp;leve specified group</br>
&ensp;<b>#who</b></br>
&ensp;&ensp;shows other clients who can receive your messages</br>
&ensp;<b>#echo <value></b></br>
&ensp;&ensp;if value 1, true, enable or yes it allows to echoing messages to client</br>
&ensp;<b>#process <value></b></br>
&ensp;&ensp;if value 1, true, enable or yes it sets message processing for incomming message</br>
