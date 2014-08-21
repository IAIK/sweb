if [ ! -f "./SWEB-flat.vmdk" ];
then
  cp "$1/utils/images/SWEB-flat.vmdk.gz" "."
  gzip -df "./SWEB-flat.vmdk.gz"
fi