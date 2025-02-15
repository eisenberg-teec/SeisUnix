#! /bin/sh
# Residual moveout analyses for the image gathers
# residual moveout is represented by z(h)^2 = z(0)^2+r1*h^2+r2*h^4/[z(0)^2+h^2]
# 	where h is half-offset
# Authors: Debashish Sarkar
# NOTE: Two reflectors need to picked to constrain the parameters.
#set -x

## Set parameters
input=migrate.data
rpicks=res.p1
cdpmin=2000
cdpmax=3800
dcdp=200
fold=40
minoffs=0
maxoffs=2000
R=1
## npicks specifies the number of picks allowed on the reflector. Change the
## the number if needed.
npicks=12
## Rmax denotes the number of reflectors on which velocity analysis is performed
Rmax=2

## Set r-parameter sampling
nr1=200 dr1=0.006 fr1=-0.6
nr2=200 dr2=0.006 fr2=-0.6

rm cig.par

pause    begin SURELANAN Demo

echo
echo "        Instructions for migration velocity analysis  "
echo

echo
echo "  Pick 12 points on the migrated image of the reflector"
echo "  This is done by placing the mouse cursor on the desired point"
echo "  and typing  s  on the keyboard. This is blind picking, you will"
echo "  not see any indication that a pick has been made."
echo

echo
echo "  When you have picked 12 points along the reflector, "
echo "  type:   q   to save the points."
echo
echo
echo "  Note: There are two reflectors in this model, so you begin the"
echo "  demo by picking the 12 points on the shallower reflector, then"
echo "  type  q." 
echo

pause   begin picking

while [ $R -le $Rmax ]
do
suwind < $input key=offset min=$minoffs max=$maxoffs | sustack |  
suximage mpicks=ref.$R title="analyzing reflector $R"

pause   ... Continue demo

echo
echo "You have now picked the 12 points on the reflector. The next plots"
echo "that come up are 2D semblance plots. You will now pick the"
echo "maximum value on each of these plots by placing the cursor on the maximum"
echo "value and then type     s      followed by  q"

pause  ... Pick the maximum on each semblance plot.

cdp=$cdpmin
while [ $cdp -le $cdpmax ]
do
	ok=false
	while [ $ok = false ]
	do
		echo "Starting Residual moveout analysis for cip $cdp"
		suwind < $input key=cdp min=$cdp max=$cdp count=$fold |
		surelanan refl=ref.$R npicks=$npicks nr1=$nr1 dr1=$dr1 fr1=$fr1 \
				nr2=$nr2 dr2=$dr2 fr2=$fr2 |
		suximage bclip=0.95 wclip=0.0 f1=$fr2 d1=$dr2 f2=$fr1 d2=$dr1 \
			label1="r2-parameter" label2="r1-parameter " \
			title="r-parameter Scan for CIP $cdp" \
			grid1=solid grid2=solid cmap="hsv8" \
			mpicks=mpicks.$cdp.$R blank=.7 key=d2 labelsize=30 titlesize=30
		pause

		/bin/echo -n "Picks OK? (y/n) " >/dev/tty
		read response
		case $response in
		n*) ok=false ;;
		*) ok=true ;;
		esac

	done </dev/tty
	cdp=`bc -l <<END
		$cdp + $dcdp
END`

done

set +x


### Combine the individual picks into a composite par file
echo "Editing pick files ..."
>$rpicks
cdp=$cdpmin
while [ $cdp -le $cdpmax ]
do
	echo -n "cip$R=$cdp," >temp
	cat temp mpicks.$cdp.$R>>$rpicks
	cdp=`bc -l <<END
		$cdp + $dcdp
END`
done

sed "s/  /,/g"<$rpicks >>cig.par

echo "dzdv par file: cig.par is ready"

### Clean up
cdp=$cdpmin
while [ $cdp -le $cdpmax ]
do
	rm mpicks.$cdp.$R 
	cdp=`bc -l <<END
		$cdp + $dcdp
END`
done
rm temp $rpicks

R=`bc -l <<END
$R + 1
END`
done
