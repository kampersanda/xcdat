#!/bin/sh

pandoc --template=template.html -o doc.html doc.md -c style.css --toc --toc-depth=2