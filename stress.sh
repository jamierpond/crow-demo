function req() {
    index=$1
    json='{"link": "https://bbc.co.uk"}'
    curl --location --request POST 'http://localhost:8080/insert' \
      --header 'Content-Type: application/json' \
      --data-raw "$json"
}

for i in {1..32000}
do
    req $i &
done
