#!/bin/bash

# Watch for changes in src/plugin.c
while inotifywait -e modify src/plugin.c; do
    echo "Plugin source changed, rebuilding..."
    make build/plugin
done