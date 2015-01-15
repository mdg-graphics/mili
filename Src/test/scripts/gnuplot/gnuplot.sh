#! /bin/csh

if ( -e "${1}.answ") then 
	set baseline="${1}.answ"
else 
	echo "baseline for ${1}.answ does not exist"
	exit 2
endif

if ( -e "${1}.jansw" ) then 
	set new_baseline="${1}.jansw"
else
	echo "new baseline for ${1}.jansw does not exist"
	exit 2
endif
rm -f $baseline.temp $new_baseline.temp

sed 's/end/\n/' $baseline > $baseline.temp
sed 's/end/\n/' $new_baseline > $new_baseline.temp
cp ${2} ${baseline}.dat
echo 'set output "'"${baseline}.jpeg"'"' >>${baseline}.dat
echo "plot '"${new_baseline}.temp"' title '"$new_baseline"' with linespoints, '"${baseline}.temp"' title '"$baseline"' with linespoints" >> ${baseline}.dat
#echo "plot " '"'"${new_baseline}.temp"'"" title '"$new_baseline"' with linespoints, " '"'"${baseline}.temp"'"" title '"$baseline"'  with linespoints" >> ${baseline}.dat

gnuplot ${baseline}.dat
rm -f $baseline.temp $new_baseline.temp





