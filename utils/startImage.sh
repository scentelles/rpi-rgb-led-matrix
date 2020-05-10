#using gifview (gifsicle package)
MS_PRESET_PATH=/home/pi/MS_presets/images
FILE=$MS_PRESET_PATH/image$1.gif

if test -f "$FILE"; then
    echo "$FILE exist"
    gifview -a $FILE &

else
    echo "###################################################################"
    echo "################ ERROR : Image File $FILE not found !!! ###########"
    echo "###################################################################"
    
fi

