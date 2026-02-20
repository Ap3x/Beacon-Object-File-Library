# BOF Project Ideas

## Evasion / Defense Bypass

### ETW & AMSI Patcher
Patch Event Tracing for Windows (ETW) and the Antimalware Scan Interface (AMSI) in-memory for the current process. Supports checking current patch state, applying patches, and reverting them -- all without writing to disk. Uses dynamic function resolution to avoid hooking detection.

### Inject AMSI Bypass (Remote Process)
Inject an AMSI bypass into a remote target process rather than the current Beacon process. Useful for preparing a process for .NET assembly execution or PowerShell usage without the Beacon itself performing suspicious AMSI-related operations.

### InlineExecute-Assembly
Run .NET assemblies directly inside the Beacon process rather than using Cobalt Strike's default fork-and-run model. Avoids the suspicious process creation and cross-process injection that EDR tools commonly detect. Includes built-in AMSI/ETW patching before loading the assembly.

## Privilege Escalation

### DCOMPotato-PrintNotify
Obtain SYSTEM privileges from a process with SeImpersonatePrivilege by passing a malicious IUnknown object to the DCOM call of PrintNotify. A potato-style privilege escalation implemented entirely as a BOF, avoiding the need to drop an executable to disk.

### Toggle Token Privileges
Dynamically add or remove token privileges on the current process. Useful for enabling privileges like SeDebugPrivilege or SeImpersonatePrivilege on demand, or for removing suspicious privileges before performing an action to reduce the detection surface.

## Credential / Token Harvesting

### Nanodump (In-Memory LSASS Dump)
Dump LSASS memory using direct system calls without ever writing the dump file to disk. The minidump is streamed directly back to the team server. Supports multiple dump techniques including MiniDumpWriteDump, a custom minidump implementation, and the silent process exit mechanism.

### WerDump (PPL Bypass via WerFaultSecure)
Bypass Protected Process Light (PPL) protection on LSASS by abusing Windows Error Reporting's WerFaultSecure.exe process. WerFaultSecure runs as a PPL and can read protected process memory, effectively using a trusted Microsoft binary to perform the dump.

### Koh (Token Capture)
Capture user logon tokens by exploiting purposeful token/logon session leakage. Waits for users to authenticate and captures their tokens in real time, allowing credential harvesting without touching LSASS or performing traditional credential dumping.

## Reconnaissance / Situational Awareness

### FindObjects (Process Handle & Module Hunter)
Use direct system calls to enumerate all running processes and identify those with specific loaded modules (e.g., `clr.dll` for .NET, `winhttp.dll`) or those holding handles to interesting objects (like LSASS token handles or file handles to sensitive paths). Useful for finding injection targets or identifying security tool processes.

### xPipe (Named Pipe Enumerator with DACLs)
Enumerate all active named pipes on the system and return their owner and full Discretionary Access Control List (DACL) permissions. Operationally valuable for identifying pipes that could be used for privilege escalation (e.g., writable pipes owned by SYSTEM) or for understanding inter-process communication channels.

### Firewall Enumerator
Interact with the Windows Firewall via COM interfaces (INetFwPolicy2) to enumerate all firewall rules, their profiles, directions, and actions. Provides complete visibility into what network traffic is permitted without spawning `netsh.exe` or other command-line utilities that are commonly monitored by EDR.

## Lateral Movement

### DCOM Lateral Movement
Implement lateral movement via Distributed COM (DCOM) using ShellWindows or ShellBrowserWindow objects. A less commonly detected lateral movement technique compared to PSExec or WMI. Supports specifying alternate credentials or using the current user's token.

## Surveillance / Data Collection

### WindowSpy (Targeted User Surveillance)
Monitor the active foreground window title and only trigger keylogging or screenshot capture when the user is interacting with specific targets (e.g., browser login pages, VPN clients, password managers, confidential documents). Produces output only when it matters, saving the operator from sifting through irrelevant data.

### Clipboard Monitor & Manipulator
Dump, edit, and continuously monitor the contents of a target's clipboard. Beyond simple clipboard theft, the manipulation capability allows for attacks like replacing cryptocurrency wallet addresses, modifying copied commands, or injecting content that the user will paste elsewhere.

## Active Directory

### BadTakeover (AD Account Takeover)
Implement the BadSuccessor technique for Active Directory account takeover. Exploits delegated permissions on AD objects to take over accounts using Shadow Credentials or Resource-Based Constrained Delegation (RBCD), providing a path to domain escalation without traditional credential theft.

## Reference Collections

- [BofAllTheThings](https://github.com/N7WEra/BofAllTheThings)
- [TrustedSec CS-Situational-Awareness-BOF](https://github.com/trustedsec/CS-Situational-Awareness-BOF)
- [Outflank C2-Tool-Collection](https://github.com/outflanknl/C2-Tool-Collection)
- [CobaltStrike_BOF_Collections](https://github.com/wsummerhill/CobaltStrike_BOF_Collections)
- [OperatorsKit](https://github.com/REDMED-X/OperatorsKit)
