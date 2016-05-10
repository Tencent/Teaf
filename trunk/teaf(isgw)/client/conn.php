<?php

echo date(DATE_RFC822)."start to conn";

$num=1000000;
if($argc >= 2)
{
    $num = $argv[1];
}

$sock = array();
$lastcidx = 1;

for ($i = 1; $i <= $num; $i++) {
    $sock[$i] = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    //socket_set_nonblock($sock);
    if (socket_connect($sock[$i],"172.25.40.94", 5693) != TRUE)
	{
		for ($j = $lastcidx; $j <= $i; $j++) {
			socket_close($sock[$j]);
		}
		$lastcidx = $j;
		echo __FILE__.":".__LINE__." close sock to $lastcidx\n";
		sleep (2); //连接失败就等一段时间再连 
	}
    usleep(2000);

}

sleep (1000);
echo __FILE__.":".__LINE__." test finish\n";

?>