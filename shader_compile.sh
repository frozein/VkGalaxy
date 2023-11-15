BASE_INPUT_DIR=./assets/shaders
BASE_INPUT_DIR_LEN=`expr ${#BASE_INPUT_DIR} + 2`

BASE_OUTPUT_DIR="./assets/spirv/"

GREEN='\033[0;32m'
NC='\033[0m'

NUM=1

walk_dir () 
{
    for pathname in "$1"/*; do
        if [ -d "$pathname" ]; then
            walk_dir "$pathname"
        else
            RELATIVE_PATH=$(echo "$pathname" | cut -c $BASE_INPUT_DIR_LEN-)

            INPUT_FILE="$pathname"
            OUTPUT_FILE=$BASE_OUTPUT_DIR$RELATIVE_PATH.spv

            if [ $INPUT_FILE -nt $OUTPUT_FILE ] || [ ! -f $OUTPUT_FILE ]; then
                OUTPUT_FILE_DIR=${OUTPUT_FILE%/*}

                if [ ! -d "$OUTPUT_FILE_DIR" ]; then
                    mkdir $OUTPUT_FILE_DIR
                fi

                echo "[${NUM}] ${GREEN}Compiling shader $INPUT_FILE ${NC}"
                glslc "$INPUT_FILE" -o "$OUTPUT_FILE"

                NUM=`expr ${#NUM} + 1`
            fi
        fi
    done
}

walk_dir $BASE_INPUT_DIR