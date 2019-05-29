flags = [ '-x', 'c' , '-std=c99', './ClangCompleter']
def Settings( **kwargs ):
  return {
    'flags': flags
  }

def FlagsForFile( filename, **kwargs ):
    return {
      'flags': flags
      }
