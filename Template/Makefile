CFLAGS=/c /GS- /std:c++20 /D_HAS_EXCEPTIONS=0 /GR-
DEBUGCFLAGS=/Zi /MTd /D_DEBUG /EHsc /std:c++20
PYTHON=python
PROJECT_NAME=__PROJECT_NAME__

!IF "$(PROCESSOR_ARCHITECTURE)" != "x86" && "$(PROCESSOR_ARCHITECTURE)" != "AMD64"
!ERROR Only x86 and AMD64 architectures are supported or the PROCESSOR_ARCHITECTURE environment variable is not set.
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "AMD64"
OUTDIR=..\x64\Release\ 
IMDIR=x64\Release\ 
DOUTDIR=..\x64\Debug\ 
DIMDIR=x64\Debug\ 
OUTEXT=.x64.o
!ELSE
OUTDIR=..\x86\Release\ 
IMDIR=x86\Release\ 
DOUTDIR=..\x86\Debug\ 
DIMDIR=x86\Debug\ 
OUTEXT=.x86.o
!ENDIF

all: *.cpp
    @$(MAKE) /A $(patsubst %.c,%.obj, $(patsubst %.cpp, %.obj, $(patsubsti %, $(IMDIR)\%, $**)))

    @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"
    copy /y "$(IMDIR)\bof.obj" "$(OUTDIR)$(PROJECT_NAME)$(OUTEXT)"

all-debug: *.cpp
    del /F /Q $(DOUTDIR)*
    $(CPP) $(DEBUGCFLAGS) /Fe"$(DOUTDIR)$(PROJECT_NAME).exe" bof.cpp ..\Core\base\mock.cpp
    del /F /Q *.obj

.cpp{$(IMDIR)}.obj:
    @if not exist "$(IMDIR)" mkdir "$(IMDIR)"
    $(CPP) $(CFLAGS) /Fo"$@" $<

    @(where $(PYTHON) >nul 2>nul \
        && $(PYTHON) --version >nul 2>nul \
        || (echo [*] Install Python to enable boflint && exit /b 0) \
    ) && $(PYTHON) ..\Core\utils\boflint.py --logformat vs --loader cs "$@"

.cpp{$(DOUTDIR)}.exe:
    @if not exist "$(DIMDIR)" mkdir "$(DIMDIR)"
    @if not exist "$(DOUTDIR)" mkdir "$(DOUTDIR)"
    $(CPP) $(DEBUGCFLAGS) /Fo$(DIMDIR) /Fd$(DIMDIR) /Fe"$@" $< ../Core/base/mock.cpp

clean:
     @if exist "$(DIMDIR)" rmdir /Q /S "$(DIMDIR)"
     @if exist "$(IMDIR)" rmdir /Q /S "$(IMDIR)"
     @if exist "$(OUTDIR)" rmdir /Q /S "$(OUTDIR)"
     @if exist "$(DOUTDIR)" rmdir /Q /S "$(DOUTDIR)"
