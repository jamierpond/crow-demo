
function req() {
    index=$1
    json='{"link":"'$index'https://reallreaasdfjakl;fdjksal;fdjsakl;fdjsakfl;adsllreallreallreallreallrealylloooooooooooooooooooooooooooooooooooooooooooooooooongggurtlllllllll.com/'$index'"}'
    curl --location --request GET '8080.pond.audio/insert' \
      --header 'Content-Type: application/json' \
      --data-raw "$json"
}


for i in {1..32000}
do
    req $i &
done
