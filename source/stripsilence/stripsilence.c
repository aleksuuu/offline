/**
 @file
 stripsilence
 */

#include "ext.h"
//#include "ext_obex.h"
//#include "ext_common.h" // contains CLAMP macro
#include "ext_buffer.h"

typedef struct _stripsilence {
    t_object f_obj;
    t_buffer_ref *f_buffer_reference;
    double f_fade_in_time; // in ms
    double f_fade_out_time;
    double f_start_time;
    double f_end_time;
    void *f_proxy1;
    void *f_proxy2;
    void *f_proxy3;
    long f_inletnum;
    void *f_outlet1;
} t_stripsilence;

void stripsilence_set(t_stripsilence *x, t_symbol *s);
void *stripsilence_new(t_symbol *s, long argc, t_atom *argv);
void stripsilence_free(t_stripsilence *x);
t_max_err stripsilence_notify(t_stripsilence *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void stripsilence_int(t_stripsilence *x, long n);

void stripsilence_int(t_stripsilence *x, long n);
void stripsilence_float(t_stripsilence *x, double f);
void stripsilence_list(t_stripsilence *x, t_symbol *s, long argc, t_atom *argv);

void stripsilence_bang(t_stripsilence *x);
void stripsilence_assist(t_stripsilence *x, void *b, long m, long a, char *s);
void stripsilence_dblclick(t_stripsilence *x);


static t_class *stripsilence_class;


C74_EXPORT void ext_main(void *r)
{
    t_class *c = class_new("stripsilence", (method)stripsilence_new, (method)stripsilence_free, sizeof(t_stripsilence), 0L, A_GIMME, 0);
    class_addmethod(c, (method)stripsilence_set, "set", A_SYM, 0);
    class_addmethod(c, (method)stripsilence_bang, "bang", 0);
    class_addmethod(c, (method)stripsilence_int, "int", A_LONG, 0);
    class_addmethod(c, (method)stripsilence_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)stripsilence_list, "list", A_GIMME, 0);
    class_addmethod(c, (method)stripsilence_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)stripsilence_dblclick, "dblclick", A_CANT, 0);
    class_addmethod(c, (method)stripsilence_notify, "notify", A_CANT, 0);
    class_addmethod(c, (method)stdinletinfo, "inletinfo", A_CANT, 0); // marks all non-left inlets as cold
    class_register(CLASS_BOX, c);
    stripsilence_class = c;
}

void stripsilence_set(t_stripsilence *x, t_symbol *s)
{
    if (!x->f_buffer_reference)
        x->f_buffer_reference = buffer_ref_new((t_object *)x, s);
    else
        buffer_ref_set(x->f_buffer_reference, s);
}

void stripsilence_int(t_stripsilence *x, long n)
{
    stripsilence_float(x, n);
}

void stripsilence_float(t_stripsilence *x, double f)
{
    switch (proxy_getinlet((t_object *)x))
    {
        case 0:
            x->f_fade_in_time = f;
            x->f_fade_out_time = f;
            stripsilence_bang(x);
            break;
        case 1:
            x->f_fade_in_time = f;
            x->f_fade_out_time = f;
            break;
        case 2:
            x->f_start_time = f;
            break;
        case 3:
            x->f_end_time = f;
            break;
    }
}

void stripsilence_list(t_stripsilence *x, t_symbol *s, long argc, t_atom *argv)
{
    if (argc != 2)
    {
        object_post((t_object *)x, "list input must contain two elements");
    }
    switch (proxy_getinlet((t_object *)x))
    {
        case 0:
            atom_arg_getdouble(&x->f_fade_in_time, 0, argc, argv);
            atom_arg_getdouble(&x->f_fade_out_time, 1, argc, argv);
            stripsilence_bang(x);
            break;
        case 1:
            atom_arg_getdouble(&x->f_fade_in_time, 0, argc, argv);
            atom_arg_getdouble(&x->f_fade_out_time, 1, argc, argv);
            break;
        case 2:
            atom_arg_getdouble(&x->f_start_time, 0, argc, argv);
            atom_arg_getdouble(&x->f_end_time, 1, argc, argv);
            break;
        default:
            object_post((t_object *)x, "inlet does not accept lists");
            break;
    }
}


void stripsilence_bang(t_stripsilence *x)
{
    if (proxy_getinlet((t_object *)x != 0))
    {
        object_post((t_object *)x, "bang only works with the leftmost inlet");
        return;
    }

    t_buffer_obj *buffer = buffer_ref_getobject(x->f_buffer_reference);
    float *tab = buffer_locksamples(buffer);
//    float *new_samples = object_alloc(sizeof(tab));

    if (!tab)
    {
        object_post((t_object *)x, "buffer is empty");
        return;
    }

    long    chan, frames, nc, fade_in_samps, fade_out_samps, total_samples,                           start_time_samps, end_time_samps;
    float   sample_rate;

    frames = buffer_getframecount(buffer);
    nc = buffer_getchannelcount(buffer);
    total_samples = frames * nc;
    sample_rate = buffer_getmillisamplerate(buffer);
    fade_in_samps = x->f_fade_in_time * sample_rate * nc;
    fade_out_samps = x->f_fade_out_time * sample_rate * nc;

    start_time_samps = x->f_start_time * sample_rate * nc;
    if (start_time_samps < 0 || start_time_samps > total_samples)
    {
        start_time_samps = 0;
    }

    end_time_samps = x->f_end_time * sample_rate * nc;
    if (end_time_samps < 0 || end_time_samps > total_samples) // hasn't been set by user or out of bounds
    {
        end_time_samps = total_samples;
    }

    long silent_samp_counter = 0;
    long new_samp_count = 0;
    
    float *new_samples = malloc(sizeof(float) * total_samples);

    for (long i = 0; i < total_samples; i++)
    {
        if (tab[i] != 0 && silent_samp_counter > 4410)
        {
            new_samp_count -= silent_samp_counter;
            silent_samp_counter = 0;
        } else if (tab[i] == 0)
        {
            silent_samp_counter++;
        }
        new_samples[new_samp_count] = tab[i];
        new_samp_count++;
    }

    for (long i = 0; i < total_samples; i++)
    {
        
        tab[i] = new_samples[i];
    }

    free(new_samples);
    buffer_setdirty(buffer);
    buffer_unlocksamples(buffer);
//    outlet_bang(x->f_outlet1);
    outlet_int(x->f_outlet1, new_samp_count);
    return;
}


// this lets us double-click on stripsilence~ to open up the buffer~ it references
void stripsilence_dblclick(t_stripsilence *x)
{
    buffer_view(buffer_ref_getobject(x->f_buffer_reference));
}

void stripsilence_assist(t_stripsilence *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_OUTLET)
        sprintf(s,"bang When Buffer Has Been Edited");
    else {
        switch (a) {
            case 0:
                sprintf(s,"bang Adds Fades to buffer~");
                break;
            case 1:
                sprintf(s,"Set Fade Time");
                break;
            case 2:
                sprintf(s, "Set Start Point");
                break;
            case 3:
                sprintf(s, "Set End Point");
                break;
        }
    }
}

void *stripsilence_new(t_symbol *s, long argc, t_atom *argv)
{
    t_stripsilence *x = object_alloc(stripsilence_class);
    t_symbol *tmp = gensym("");
    switch (argc)
    {
        case 0:
            object_post((t_object *)x, "requires at least one argument");
            break;
        case 1:
            atom_arg_getsym(&tmp, 0, argc, argv);
            x->f_fade_in_time = 20;
            x->f_fade_out_time = 20;
            break;
        case 2:
            atom_arg_getsym(&tmp, 0, argc, argv);
            atom_arg_getdouble(&x->f_fade_in_time, 1, argc, argv);
            x->f_fade_out_time = x->f_fade_in_time;
            break;
        case 3:
            atom_arg_getsym(&tmp, 0, argc, argv);
            atom_arg_getdouble(&x->f_fade_in_time, 1, argc, argv);
            atom_arg_getdouble(&x->f_fade_out_time, 2, argc, argv);
            break;
        default:
            object_post((t_object *)x, "received too many arguments");
    }
    stripsilence_set(x, tmp);
    x->f_start_time = 0;
    x->f_end_time = -1; // when a bang has been received check to see if end time is -1 in bang; if it is, use buffer length

    // creating the proxies (inlets) from right to left so that their numbering would be correct
    x->f_proxy3 = proxy_new(x, 3, &x->f_inletnum);
    x->f_proxy2 = proxy_new(x, 2, &x->f_inletnum);
    x->f_proxy1 = proxy_new(x, 1, &x->f_inletnum);
    x->f_outlet1 = bangout((t_object *)x);
    return x;
}


void stripsilence_free(t_stripsilence *x)
{
    object_free(x->f_buffer_reference);
    object_free(x->f_proxy1);
    object_free(x->f_proxy2);
    object_free(x->f_proxy3);
}

t_max_err stripsilence_notify(t_stripsilence *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
    return buffer_ref_notify(x->f_buffer_reference, s, msg, sender, data);
}
