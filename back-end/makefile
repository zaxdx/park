
EXE = park
DMP = dump
STP = strip
CXX = g++

#-fno-rtti

CXXFLAGS  = -std=c++23 -march=native -mavx -m64 -Wall -Wextra -Wconversion  -flto=auto  -ffast-math
CXXFLAGS += -Icore/base -Icore/gpio -Icore/srvr -Icore/view

CXXLIBS   = -lm -lv4l2
DBG       = -O2 -g0
FNL       =
OSRC      = main.cpp drvr.cpp nccp.cpp view.cpp srvr.cpp

OBJS=$(addprefix .temp/, $(addsuffix .o, $(basename $(notdir $(OSRC)))))
OBJX=$(addprefix .temp/, $(addsuffix .o, $(basename $(notdir $(OSRX)))))

all: $(EXE)
dmp: $(DMP)

$(EXE): $(OBJS) $(OBJX)
	@echo "Gerando $@: $@"
	@$(CXX) -o $@ $^ $(CXXFLAGS) $(CXXLIBS) $(DBG) $(FNL)
	@$(STP) $@
	@printf "\e[00;00m\n"

$(DMP): $(OBJD)
	@printf "\e[00;32mGerando $@: \e[00;37m$@"
	@echo "Gerando $@: $@"
	@$(CXX) -o $@ $^ $(CXXFLAGS) $(CXXLIBS) $(DBG)
	@printf "\e[00;00m\n"

.temp/%.o:%.cpp
	@mkdir -p .temp
	@printf "\e[00;32mCompilando [\e[00;33m  main  \e[00;32m] modulo: \e[00;37m$<\e[00;00m\n"
#	@echo "Compilando [  main  ] modulo: $<"
	@$(CXX) $(CXXFLAGS) $(DBG) -c -o $@ $<

.temp/%.o:core/base/%.cpp
	@mkdir -p .temp
	@printf "\e[00;32mCompilando [\e[00;34m  base  \e[00;32m] modulo: \e[00;37m$<\e[00;00m\n"
#	@echo "Compilando [  xwin  ] modulo: $<"
	@$(CXX) $(CXXFLAGS) $(DBG) -c -o $@ $<

.temp/%.o:core/view/%.cpp
	@mkdir -p .temp
	@printf "\e[00;32mCompilando [\e[00;35m  view  \e[00;32m] modulo: \e[00;37m$<\e[00;00m\n"
#	@echo "Compilando [  xwin  ] modulo: $<"
	@$(CXX) $(CXXFLAGS) $(DBG) -c -o $@ $<

.temp/%.o:core/srvr/%.cpp
	@mkdir -p .temp
	@printf "\e[00;32mCompilando [\e[00;36m  srvr  \e[00;32m] modulo: \e[00;37m$<\e[00;00m\n"
#	@echo "Compilando [  xwin  ] modulo: $<"
	@$(CXX) $(CXXFLAGS) $(DBG) -c -o $@ $<

clean:
	@rm -f $(EXE) $(DMP) $(OBJS) $(OBJX) *~
	@printf "\e[00;32m--=| basic clean |=--\e[00;00m\n"
#	@echo '--=| basic clean |=--'

fclean:
	@rm -f $(EXE) $(OBJS) $(OBJG) $(OBJD) $(DMP) *~
	@printf "\e[00;32m--=| full clean |=--\e[00;00m\n"
#	@echo '--=| full clean |=--'
