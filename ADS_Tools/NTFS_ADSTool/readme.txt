NTFS Alternate Data Streams Tool
Copyright(C) 2007-2008 Whislter Lee

Usage: NTFS_ADSTool.exe /c src_stream dest_stream
       NTFS_ADSTool.exe /d stream_path
       NTFS_ADSTool.exe /l stream_path


a.	Switch /c	Copy a stream to another stream.

	If the stream is not specified, the command assumes the unnamed stream(the main stream), you might find the result to be totally unexpected.
	Beware of the two cases:
	
	1. Unnamed stream to unnamed stream: 
	Treated as a file operation, that is all the named streams also get copied. If the target file exists, it is replaced. 

	2. Named stream to unnamed stream:
	Also treated as a file operation, although only one stream gets copied. Existing target file gets deleted, 
	so instead of replacing the unnamed stream as you might expect, the function replaces the whole target file with a new single-stream file. 

b.	Switch /d	Delete a stream.
	You can not delete the unnamed stream alone(the main stream), deleting it also delets all the other alternate streams if there's any.
	
c.	Switch /l	List a file's or a directory's streams if any.