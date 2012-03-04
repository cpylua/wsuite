If the stream is not specified, the command assumes the unnamed stream(the main stream), if doing so, you might find the result to be totally unexpected.

Beware of the two cases:

1. Unnamed stream to unnamed stream: 
Treated as a file operation, that is all the named streams also get copied. If the target file exists, it is replaced. 

2. Named stream to unnamed stream:
Also treated as a file operation, although only one stream gets copied. Existing target file gets deleted, 
so instead of replacing the unnamed stream as you might expect, the function replaces the whole target file with a new single-stream file. 
