while IFS= read -r line
do
  echo "${line#*]}"
done | convbin --input /dev/stdin --output TAS.8xv --iformat csv --oformat 8xv --name TAS