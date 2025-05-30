define target remote
target extended-remote $arg0
symbol-file build/crawl.elf
monitor reset shellhalt
load build/crawl.elf
end
