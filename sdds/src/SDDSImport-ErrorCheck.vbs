' *************** Global variables ****************

Class CWScriptWrapper
	Public m_bQuiting
	Public m_nRetVal
	Private Sub Class_Initialize
		m_nRetVal = 1
		m_bQuiting = False
	End Sub
	Public Sub Quit(nRetVal)
		m_nRetVal = nRetVal
		m_bQuiting = True
		WScript.Quit(m_nRetVal)
	End Sub
End Class

Dim oWScriptWrapper

Class CThisApp
	Private Sub Class_Initialize
		Set oWScriptWrapper = New CWScriptWrapper
		Main()
		Err.Clear
	End Sub
	Private Sub Class_Terminate
		Dim nErrNum : nErrNum = Err.Number
		If 0 = nErrNum Then
			Exit Sub
		ElseIf oWScriptWrapper.m_bQuiting Then
			Exit Sub
		Else
			WScript.Quit(nErrNum)
		End If
	End Sub
End Class

Dim g_sSrcLogFile : g_sSrcLogFile = ""

Dim g_oFSO : Set g_oFSO = CreateObject("Scripting.FileSystemObject")

Dim oThisApp : Set oThisApp = New CThisApp : Set oThisApp = Nothing

' ************** Main program ***********************

Sub Main()
	ReadArguments

	CheckLogFile

	ExitScript(0)
End Sub


' ************** Helper routines ********************


Sub ReadArguments()
    
	If WScript.Arguments.Count = 0 Then
		LogErr "Incorrect number of parameters"
		DisplayUsage()
		ExitScript(1)
	End If

	Dim arg
	arg = WScript.Arguments(0)	
	If  LCase(arg) = "usage" or _
	    LCase(arg) = "help" or _
	    Left(arg, 1) = "?" or _
	    arg = "/?" or _
	    arg = "-?" Then
	    DisplayUsage()
	    ExitScript(0)
	End If

	' Read all command line arguments into global variables
		
	Dim argIndex : argIndex = 0
	
	Do while argIndex < WScript.Arguments.Count
		arg = WScript.Arguments(argIndex)
				
		If LCase(arg) = "-logfile" Then
			argIndex = argIndex + 1
			g_sSrcLogFile = WScript.Arguments(argIndex)
		Else
			LogErr "Invalid argument [" + arg + "]"
			DisplayUsage()
			ExitScript(1)
		End If
		
		argIndex = argIndex + 1
	Loop
End Sub


Sub DisplayUsage()
	WScript.Echo "Usage:"
	WScript.Echo "    cscript.exe //nologo SDDSImport-ErrorCheck.vbs [Parameters]"
	WScript.Echo ""
	WScript.Echo "    [Parameters]"
	WScript.Echo ""	
	WScript.Echo "    -?|/?|?|usage|help <Displays this usage information>"	
	WScript.Echo "    -logfile  Path to the SDDS import log file to check for errors (mandatory)"
End Sub	


Sub CheckLogFile
	' Validate source log file
	If Len(g_sSrcLogFile) < 1 Then
		LogErr "Log file name cannot be empty"
		ExitScript(1)
	End If
	If g_oFSO.FileExists(g_sSrcLogFile) = False Then
		LogErr "Log file [" + g_sSrcLogFile + "] does not exist"
		ExitScript(1)
	End If
	
	bSuccess = False
	
	Set oFile = g_oFSO.OpenTextFile(g_sSrcLogFile)
	Do Until oFile.AtEndOfStream
		line = oFile.ReadLine
		If InStr(line, "All operations appear to have completed successfully") <> 0 Then
			bSuccess = True
			Exit Do
		End If
	Loop
	oFile.Close
	
	If bSuccess = False Then
		LogErr "Success message was not found in log file"
		ExitScript(1)
	End If
End Sub


Sub ExitScript(ByVal nExitCode)
	oWScriptWrapper.Quit(nExitCode)
End Sub


' Logs a warning
Sub LogWar(ByVal sLogMsg)
	WScript.Echo "[" + CStr(Now) + "] " + "WAR: " + sLogMsg
End Sub


' Logs an error
Sub LogErr(ByVal sLogMsg)
	WScript.StdErr.WriteLine "[" + CStr(Now) + "] " + "ERR: " + sLogMsg
End Sub


' Logs information
Sub LogInf(ByVal sLogMsg)
	WScript.Echo "[" + CStr(Now) + "] " + "INF: " + sLogMsg
End Sub


' Logs a secret
Sub LogSecret(ByVal sLogMsg)
	' Uncommenting the line below will cause sensitive information
	' like passwords etc. to be logged. Therefore uncomment it only for troubleshooting purposes
	' and don't forget to comment it out again when done.
	'LogInf sLogMsg
End Sub
