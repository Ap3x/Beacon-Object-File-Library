# Beacon Object File Library

This repo contains a library of Beacon Object Files (BOFs) that can be used in Cobalt Strike. I plan to add more BOFs as time goes on. 

This solution uses a modified version of [Cobalt Strike/bof-vs](https://github.com/Cobalt-Strike/bof-vs) repo to fit into a solution.

## Unit Testing

Each BOF includes unit tests with mocked functions to ensure proper code coverage Notice each BOF has a separate function outside of the main `go()` that contains all the Windows API calls. This is so that when we unit test we can easily mock the output without having to mock all the Windows APIs functions.

## Additional Testing

If you would like to test this BOF in a COFF Loader I recommend using my COFF Loader: [Ap3x/Coff-Loader](https://github.com/Ap3x/COFF-Loader) or [TrustedSec's COFF Loader](https://github.com/trustedsec/COFFLoader).

## TODO
- [] Add CNA files so that they can be imported into Cobalt Strike