# Makefile to be used with "nmake".
INCLS=bs1770_ctx.h bs1770.h bs1770_types.h biquad.h
OBJS=biquad.obj bs1770_version.obj bs1770_ctx.obj bs1770_nd.obj bs1770_stats.obj bs1770.obj bs1770_aggr.obj bs1770_hist.obj bs1770_a85.obj bs1770_r128.obj bs1770_default.obj bs1770_ctx_add_samples_i_i16.obj bs1770_ctx_add_samples_i_i32.obj bs1770_ctx_add_samples_i_f32.obj bs1770_ctx_add_samples_i_f64.obj bs1770_ctx_add_samples_p_i16.obj bs1770_ctx_add_samples_p_i32.obj bs1770_ctx_add_samples_p_f32.obj bs1770_ctx_add_samples_p_f64.obj bs1770_ctx_add_sample_i16.obj bs1770_ctx_add_sample_i32.obj bs1770_ctx_add_sample_f32.obj bs1770_ctx_add_sample_f64.obj bs1770_nd_add_samples_i_i16.obj bs1770_nd_add_samples_i_i32.obj bs1770_nd_add_samples_i_f32.obj bs1770_nd_add_samples_i_f64.obj bs1770_nd_add_samples_p_i16.obj bs1770_nd_add_samples_p_i32.obj bs1770_nd_add_samples_p_f32.obj bs1770_nd_add_samples_p_f64.obj bs1770_nd_add_sample_i16.obj bs1770_nd_add_sample_i32.obj bs1770_nd_add_sample_f32.obj bs1770_nd_add_sample_f64.obj bs1770_add_samples_p_i16.obj bs1770_add_samples_p_i32.obj bs1770_add_samples_p_f32.obj bs1770_add_samples_p_f64.obj bs1770_add_samples_i_i16.obj bs1770_add_samples_i_i32.obj bs1770_add_samples_i_f32.obj bs1770_add_samples_i_f64.obj bs1770_add_sample_i16.obj bs1770_add_sample_i32.obj bs1770_add_sample_f32.obj bs1770_add_sample_f64.obj
CC=cl /nologo
all: example2.exe example1.exe lib1770.lib lib1770.dll lib1770.dll.lib
clean:
	del example2.exe example1.exe *.dll *.exp *.lib *.obj
example2.exe: example2.obj lib1770.dll.lib
	link /nologo /out:example2.exe /subsystem:console /machine:x86 example2.obj lib1770.dll.lib
example1.exe: example1.obj lib1770.dll.lib
	link /nologo /out:example1.exe /subsystem:console /machine:x86 example1.obj lib1770.dll.lib
lib1770.lib: $(OBJS)
	lib /nologo /out:lib1770.lib $(OBJS)
lib1770.dll lib1770.dll.lib: $(OBJS) lib1770.def $(OBJS)
	link /nologo /out:lib1770.dll /dll /machine:x86 /def:lib1770.def /implib:lib1770.dll.lib $(OBJS)
example2.obj example1.obj $(OBJS): $(INCLS)
