import sys
import os
import time

class Logger(object):
    def __init__( self ):
        log_path = os.path.join( "log", "log.txt" )

        if not os.path.exists( "log" ): os.mkdir( "log" )

        if os.path.exists( log_path ): os.remove( log_path )

        self.log_file = open( log_path, "a" )
        self.VERBOSE = (os.environ.get("VERBOSE_WAREHOUSE_GENERATION", "0") == "1")

    def __del__( self ):
        self.log_file.close()

    def traceMessage(self, strLogMessage ):
        if self.VERBOSE:
            sys.stdout.write( strLogMessage + "\n" )
        self.log_file.write( time.strftime( "(%Y-%m-%d %H:%M:%S) [TRACE] : " ) + strLogMessage + "\n" )

    def statusMessage( self, strLogMessage ):
        sys.stdout.write( strLogMessage + "\n" )
        self.log_file.write( time.strftime( "(%Y-%m-%d %H:%M:%S) [STATUS] : " ) + strLogMessage + "\n" )

    def errorMessage( self, strErrorDescription, error = None ):
        if error:
            errorMessage = strErrorDescription + " error: " + str( error )
        else:
            errorMessage = strErrorDescription

        try:
            sys.stderr.write( errorMessage + "\n" )
            self.log_file.write( time.strftime( "(%Y-%m-%d %H:%M:%S) [ERROR] : " ) + errorMessage + "\n" )
        except SystemExit, e:
            sys.stderr.write( "stderr contains: " + sys.stderr.getvalue() )
