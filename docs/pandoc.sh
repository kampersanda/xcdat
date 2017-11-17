#!/bin/sh

pandoc --template=template.html -o index.html document.md -c style.css --toc --toc-depth=2