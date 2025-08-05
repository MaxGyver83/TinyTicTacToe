#!/bin/sh
set -e

generate_filename() {
    echo "$1" | tr ' ' '_' | tr -d ':!?' | tr '[:upper:]' '[:lower:]'
}

create_sprite() {
    text="$1"
    dpi="${2:-150}"
    margin="${3:-1}"
    basename="${4:-$(generate_filename "$text")}"
    pango-view --background=transparent --dpi=$dpi --margin=$margin -qo "$basename".png <(echo "$text")
    pngtopam -alpha "$basename.png" > "$basename".pgm && rm "$basename.png"
    echo "Created $basename.pgm"
}

mkdir -p app/src/main/assets/sprites
cd app/src/main/assets/sprites
rm ./*

create_sprite 'has won!'
create_sprite 'Draw'
create_sprite 'Next turn:'
create_sprite 'Level:' 240
create_sprite 'Very easy'
create_sprite 'Easy'
create_sprite 'Medium'
create_sprite 'Hard'
create_sprite 'Very hard'

for i in $(seq 0 9); do
    create_sprite "$i"
done
