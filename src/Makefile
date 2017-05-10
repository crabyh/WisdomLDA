ghc: 
	mpic++ -O3 -std=c++11 -fopenmp -Wall -g -o lda dense_model.cpp lda_worker.cpp main.cpp

latedays: 
	mpic++ -Wl,-rpath,/opt/gcc/4.9.2/lib64 -O3 -std=c++11 -Wno-literal-suffix -fopenmp -Wall -g -o lda dense_model.cpp lda_worker.cpp main.cpp

clean:
	rm -rf lda *~ *.*~

# TARGET = lda
# OBJS += lda_model.o
# OBJS += lda_worker.o
# OBJS += main.o

# CC = mpicc
# #CFLAGS = -Wall -Werror -DDEBUG -g # debug flags
# CFLAGS = -Wall -Werror -g -O3 # release flags
# CFLAGS += -MMD -MP
# LDFLAGS += $(LIBS)

# default:	$(TARGET)
# all:		$(TARGET)

# $(TARGET):	$(OBJS)
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# %.o: %.c
# 	$(CC) $(CFLAGS) -c -o $@ $<

# DEPS = $(OBJS:%.o=%.d)
# -include $(DEPS)

# clean:
# 	rm $(TARGET) $(OBJS) $(DEPS) latedays.qsub.* || true
