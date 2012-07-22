<?PHP

$last = array();
$lastc=0;

        while(1) {
                unset($return);
                exec('./hidtool read', $return); //Read from sensor
                $time=time()*1000;
                $return=$return[0];
                $return=explode(' 0x', $return);
                $return[0]=str_replace('0x', '', $return[0]);
                $a[0] = $return[1].$return[0];
                $a[1] = $return[3].$return[2];
                $a[2] = $return[5].$return[4];
                //var_dump($a);
                $a[0] = hexdec($a[0]);
                $a[1] = hexdec($a[1]);
                $a[2] = hexdec($a[2]);
		
		if($a[0] < 100 || $a[1] < 100 ) { //Bogous values?!
			exec('./hidtool write'); //Reset device
			sleep(2);
			break;
		}

                if($a[0] < 100 || $a[0] >= 925) {
                        $a[0] = 0;
                }

                if($a[1] < 100 || $a[1] >= 1023) {
                        $a[1] = 0;
                }

                if($a[2] < 500) $a[2] = 12;

		$r1=101; //Voltage devider R1
		$r2=221; //Voltage devider R2

                if($a[2] != 12) $a[2] = ((5/1023*$a[2])/$r1*($r1+$r2));
                if($a[1] != 0) $a[1] = ((((5/1023*$a[1])-0.5)*10-20)*-1)*1; 
                if($a[0] != 0) $a[0] = ((((5/1023*$a[0])-0.5)*10-20)*-1)*1;
                $a[3] = $a[0]+$a[1]+.15;

		if($a[3] > 16 { //>190W Solar power - we dont have such a large system
			exec('./hidtool write'); //Request Reset
			sleep(2);
			break;
		}


                //var_dump($a);
                echo 'Out: '.round($a[1],3).'A - Battery: '.round($a[0],3).'A - Solar: '.round($a[3],3).'A - Voltage: '.round($a[2],3).'V'."\n";
                $a[0]*=$a[2];
                $a[1]*=$a[2];
                $a[3]*=$a[2];
		if($a[3] <2) $a[3]=0;
                echo 'Out: '.round($a[1],3).'W - Battery: '.round($a[0],3).'W - Solar: '.round($a[3],3).'W'."\n";
		if($lasttime != time() && time()%5 == 0) {
	                if($a[1] != 0 && $a[1] != $last[1] || $lastc>=5) {
	                        $null=file_get_contentsp("http://192.168.101.2/arc/power/vz/volkszaehler.org/htdocs/middleware.php/data/161599c0-9c4f-11e1-82b2-970aae4b0c5c.json?operation=add&value=".$a[1].'&ts='.$time);
				$last[1] = $a[1];
			}
	                if($a[0] != 0 && $a[0] != $last[0] || $lastc>=5) { 
	                        $null=file_get_contentsp("http://192.168.101.2/arc/power/vz/volkszaehler.org/htdocs/middleware.php/data/2294a640-9c4f-11e1-9623-2bd1ee8c000e.json?operation=add&value=".$a[0].'&ts='.$time);
				$last[0] = $a[0];
			}
	                if($a[3] != 0 && $a[3] != $last[3] || $lastc>=5) { 
	                        $null=file_get_contentsp("http://192.168.101.2/arc/power/vz/volkszaehler.org/htdocs/middleware.php/data/86733ee0-9c4f-11e1-bed9-939e9daa99e3.json?operation=add&value=".$a[3].'&ts='.$time);
				$last[3] = $a[3];
			}
			if($a[2] != 0 && $a[2] != $last[2] || $lastc>=5) {
				$null=file_get_contentsp("http://192.168.101.2/arc/power/vz/volkszaehler.org/htdocs/middleware.php/data/cf5b89d0-b4a2-11e1-89ac-651d86b3169b.json?operation=add&value=".$a[2].'&ts='.$time);
				$last[2] = $a[2];
			}
		}
		$lasttime=$time;
		$lastc++;
		if($lastc > 5) $lastc=0;
                sleep(1);
        }

function file_get_contentsp($url) {
#	echo $url."\n";
	return file_get_contents($url);
}
?>
