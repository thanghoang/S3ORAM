ProjectName            :=S3ORAM
ConfigurationName      :=Debug
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Thang Hoang
Date                   :=04/09/17
CodeLitePath           :="/home/thanghoang/.codelite"
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="S3ORAM.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -lntl -lgmp -lzmq -lpthread
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O0 -std=c++11 -Wall -libstdc++ -c -fPIC $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/Utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/S3ORAM.cpp$(ObjectSuffix) $(IntermediateDirectory)/ClientS3ORAM.cpp$(ObjectSuffix) $(IntermediateDirectory)/ServerS3ORAM.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM "main.cpp"

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) "main.cpp"

$(IntermediateDirectory)/Utils.cpp$(ObjectSuffix): Utils.cpp $(IntermediateDirectory)/Utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "Utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Utils.cpp$(DependSuffix): Utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Utils.cpp$(DependSuffix) -MM "Utils.cpp"

$(IntermediateDirectory)/Utils.cpp$(PreprocessSuffix): Utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Utils.cpp$(PreprocessSuffix) "Utils.cpp"

$(IntermediateDirectory)/S3ORAM.cpp$(ObjectSuffix): S3ORAM.cpp $(IntermediateDirectory)/S3ORAM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "S3ORAM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/S3ORAM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/S3ORAM.cpp$(DependSuffix): S3ORAM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/S3ORAM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/S3ORAM.cpp$(DependSuffix) -MM "S3ORAM.cpp"

$(IntermediateDirectory)/S3ORAM.cpp$(PreprocessSuffix): S3ORAM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/S3ORAM.cpp$(PreprocessSuffix) "S3ORAM.cpp"

$(IntermediateDirectory)/ClientS3ORAM.cpp$(ObjectSuffix): ClientS3ORAM.cpp $(IntermediateDirectory)/ClientS3ORAM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "ClientS3ORAM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ClientS3ORAM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ClientS3ORAM.cpp$(DependSuffix): ClientS3ORAM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ClientS3ORAM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ClientS3ORAM.cpp$(DependSuffix) -MM "ClientS3ORAM.cpp"

$(IntermediateDirectory)/ClientS3ORAM.cpp$(PreprocessSuffix): ClientS3ORAM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ClientS3ORAM.cpp$(PreprocessSuffix) "ClientS3ORAM.cpp"

$(IntermediateDirectory)/ServerS3ORAM.cpp$(ObjectSuffix): ServerS3ORAM.cpp $(IntermediateDirectory)/ServerS3ORAM.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "ServerS3ORAM.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ServerS3ORAM.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ServerS3ORAM.cpp$(DependSuffix): ServerS3ORAM.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ServerS3ORAM.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ServerS3ORAM.cpp$(DependSuffix) -MM "ServerS3ORAM.cpp"

$(IntermediateDirectory)/ServerS3ORAM.cpp$(PreprocessSuffix): ServerS3ORAM.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ServerS3ORAM.cpp$(PreprocessSuffix) "ServerS3ORAM.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


