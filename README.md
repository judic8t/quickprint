# quickprint

A simple, useful printing tool for linux.

Most modern printers support PDF and Microsoft office file types.
As long as you have a relatively recent printer, printing issues
usually result from a failure in discovery or delivery, not with
the file format. This program uses an IPP print-job request to
send a job over the network with an HTTP POST request.

Since the program uses only built-in libraries, you can compile
with:

gcc quickprint.c

or however you wish to compile C code.

Usage:
1. Connect your printer to the network and figure out it's IP address
   Look in the user manual for help with getting the IP
2. Convert your file to PDF, or Word doc if you can't do PDF.
3. Use quickprint to send the file and print!

If it can't reach the printer, you may need to enable IPP or port 631
on the printer. Once again, look at the user manual for help.
If it's an option, enabling Airprint or Mopria should accomplish this.

Quickprint usage: use --help or -h.
