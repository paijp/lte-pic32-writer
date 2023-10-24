<?php

#$urlz = "https://yourteam.zulipchat.com/api/v1/external/slack_incoming?api_key=@@@@yourkey@@@@";
#$urlz .= "&stream=log";
#date_default_timezone_set("Asia/Tokyo");

include("env.php");


if (($imei = @$_SERVER["HTTP_X_SORACOM_IMEI"]) === null)
	die();
if (!preg_match('/^[0-9A-Fa-f]+$/', $imei))
	die();

if (is_dir($imei))
	;
else if (file_exists($imei))
	die();
else
	mkdir($imei, 0770);


$sendto = "";
if (is_readable($fn = $imei."/sendto.txt"))
	$sendto = trim(file_get_contents($fn));


function	send($s = "", $topic = "download", $url = "")
{
	if ($s == "")
		return;
	if ($url == "")
		return;
	$list = array("text" => $s);
	$a = array(
		"http" => array(
			"method" => "POST",
			'protocol_version' => 1.1,
			"header" => "Connection: close\r\nContent-Type: application/json\r\n",
			"content" => json_encode($list)
		)
	);
	for (;;) {
		if (file_get_contents($url."&topic={$topic}", FALSE, stream_context_create($a)) != "")
			break;
		sleep(5);
	}
}


if (@$_GET["p"] === null) {
	$a = json_decode(file_get_contents("php://input"), true);
	file_put_contents("{$imei}/log".date("ymd").".txt", $s = base64_decode(@$a["payload"]), FILE_APPEND);
	if ($sendto == "")
		send(preg_replace('/[^\012\040-\176]+/', '?', $s), "log-{$imei}", $urlz);
	else
		send(preg_replace('/[^\012\040-\176]+/', '?', $s), "log-{$imei}", $sendto);
	die();
}


function	uw2bin($l)
{
	$s = chr($l & 0xff);
	$s .= chr(($l >> 8) & 0xff);
	$s .= chr(($l >> 16) & 0xff);
	$s .= chr(($l >> 24) & 0xff);
	return $s;
}


class	writebuf {
	var	$baseaddr;
	var	$data;
	function	__construct($baseaddr) {
		$this->baseaddr = $baseaddr;
		for ($i=0; $i<0x400; $i++)
			$this->data[$i] = 0xff;
	}
	function	set($addr, $data) {
		if ($addr < $this->baseaddr)
			die(sprintf("addr(%08x) baseaddr(%08x)\n", $addr, $this->baseaddr));
		if ($addr >= $this->baseaddr + 0x400) 
			die(sprintf("addr(%08x) baseaddr(%08x)\n", $addr, $this->baseaddr));
		$this->data[$addr - $this->baseaddr] = $data;
	}
	function	put() {
		$s = "";
		for ($i=0; $i<0x400; $i++)
			$s .= chr($this->data[$i]);
		$s .= uw2bin($this->baseaddr);
		return $s;
	}
}
$writebuflist = array();


if (!is_readable($fn = "{$imei}/download.hex"))
	die();

$addrh = 0;
$content = file_get_contents($fn);
foreach (preg_split("/\r\n|\r|\n/", $content) as $line) {
	foreach (explode(":", trim($line)) as $key => $block) {
		if ($key == 0)
			continue;
		$block = strtolower($block);
		$list = array();
		$pos = 0;
		while ($pos < strlen($block) - 1) {
			$c = strpos("0123456789abcdef", substr($block, $pos++, 1)) << 4;
			$c |= strpos("0123456789abcdef", substr($block, $pos++, 1));
			$list[] = $c;
		}
		if (count($list) < 4)
			continue;
		switch ($list[3]) {
			case	0:
				$addr = $addrh | ($list[1] << 8) | $list[2];
				for ($i=0; $i<$list[0]; $i++) {
					$addr0 = $addr & 0xfffffc00;
					if (($obj = @$writebuflist[$addr0]) === null)
						$obj = $writebuflist[$addr0] = new writebuf($addr0);
					$obj->set($addr++, $list[4 + $i]);
				}
				break;
			case	1:
if ((0)) {
	foreach ($writebuflist as $key => $obj)
		printf(" %08x\n", $key);
}
				$s = "";
				$pagesize = 64;
				$page = @$_GET["p"] + 0;
				
				$pos = 0;
				foreach ($writebuflist as $key => $obj) {
					if (floor($pos++ / $pagesize) == $page)
						$s .= $obj->put();
				}
send("download: #{$page} len(".strlen($s).") ".sha1($content)." imei({$imei})", "download", $urlz);
				header("Content-Length: ".strlen($s));
				header("Content-Type: application/octet-stream");
				print $s;
				die();
			case	4:		# address-high
				if (count($list) < 6)
					continue 2;
				$addrh = ($list[4] << 24) | ($list[5] << 16);
				
				# # # # writebufsize = 0
				
				break;
		}
	}
}


