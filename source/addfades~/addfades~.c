/**
 @file
 addfades~ - SDK example of an object which accesses an MSP buffer~

 @ingroup    examples
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_common.h" // contains CLAMP macro
//#include "ext_strings.h"
//#include "ext_symobject.h"
//#include "commonsyms.h"
#include "z_dsp.h"
#include "ext_buffer.h"


typedef struct _addfades {
    t_pxobject l_obj;
    t_buffer_ref *l_buffer_reference;
    long l_fade_in_time; // in ms
    long l_fade_out_time;
    void *l_outlet1;
} t_addfades;

void addfades_set(t_addfades *x, t_symbol *s);
void *addfades_new(t_symbol *s, long argc, t_atom *argv);
void addfades_free(t_addfades *x);
t_max_err addfades_notify(t_addfades *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void addfades_in1(t_addfades *x, long n);
void addfades_bang(t_addfades *x);
void addfades_assist(t_addfades *x, void *b, long m, long a, char *s);
void addfades_dblclick(t_addfades *x);


static t_class *addfades_class;


C74_EXPORT void ext_main(void *r)
{
    t_class *c = class_new("addfades~", (method)addfades_new, (method)addfades_free, sizeof(t_addfades), 0L, A_GIMME, 0);
    class_addmethod(c, (method)addfades_set, "set", A_SYM, 0);
    class_addmethod(c, (method)addfades_in1, "in1", A_LONG, 0);
    class_addmethod(c, (method)addfades_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)addfades_dblclick, "dblclick", A_CANT, 0);
    class_addmethod(c, (method)addfades_notify, "notify", A_CANT, 0);
    class_addmethod(c, (method)addfades_bang, "bang", 0);
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    addfades_class = c;
}

void addfades_set(t_addfades *x, t_symbol *s)
{
    if (!x->l_buffer_reference)
        x->l_buffer_reference = buffer_ref_new((t_object *)x, s);
    else
        buffer_ref_set(x->l_buffer_reference, s);
}

void addfades_in1(t_addfades *x, long n)
{
//    if (n)
//        x->l_chan = MAX(n, 1) - 1;
//    else
//        x->l_chan = 0;
}

void addfades_bang(t_addfades *x)
{
    t_float         *tab;
    long            chan, frames, nc, fade_in_samps, fade_out_samps, total_samples;
    float           sample_rate;
    t_buffer_obj    *buffer = buffer_ref_getobject(x->l_buffer_reference);

    tab = buffer_locksamples(buffer);
    if (!tab)
        goto zero;
    frames = buffer_getframecount(buffer);
    nc = buffer_getchannelcount(buffer);
    total_samples = frames * nc;
    sample_rate = buffer_getmillisamplerate(buffer);
    fade_in_samps = x->l_fade_in_time * sample_rate * nc;
    fade_out_samps = x->l_fade_out_time * sample_rate * nc;
    
    for (long i = 0; i < fade_in_samps; i++)
    {
        tab[i] *= i / (float)fade_in_samps;
    }
    for (long i = 0; i < fade_out_samps; i++)
    {
        tab[total_samples - i] *= i / (float)fade_out_samps;
    }
    buffer_setdirty(buffer);
    buffer_unlocksamples(buffer);
    outlet_bang(x->l_outlet1);
    return;
zero:
    object_post((t_object *)x, "buffer is empty");
}




//void addfades_dsp64(t_addfades *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
//{
//    dsp_add64(dsp64, (t_object *)x, (t_perfroutine64)addfades_perform64, 0, NULL);
//}


// this lets us double-click on addfades~ to open up the buffer~ it references
void addfades_dblclick(t_addfades *x)
{
    buffer_view(buffer_ref_getobject(x->l_buffer_reference));
}

void addfades_assist(t_addfades *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_OUTLET)
        sprintf(s,"(signal) Sample Value at Index");
    else {
        switch (a) {
        case 0:    sprintf(s,"(signal) Sample Index");    break;
        case 1:    sprintf(s,"Audio Channel In buffer~");    break;
        }
    }
}

void *addfades_new(t_symbol *s, long argc, t_atom *argv)
{
    t_addfades *x = object_alloc(addfades_class);
    dsp_setup((t_pxobject *)x, 0);
//    intin((t_object *)x,1);
//    outlet_new((t_object *)x, "signal");
    t_symbol *tmp = gensym("");
    switch (argc)
    {
        case 0:
            object_post((t_object *)x, "requires at least one argument");
            break;
        case 1:
            atom_arg_getsym(&tmp, 0, argc, argv);
            x->l_fade_in_time = 20;
            x->l_fade_out_time = 20;
            break;
        case 2:
            atom_arg_getsym(&tmp, 0, argc, argv);
            atom_arg_getlong(&x->l_fade_in_time, 1, argc, argv);
            x->l_fade_out_time = x->l_fade_in_time;
            break;
        case 3:
            atom_arg_getsym(&tmp, 0, argc, argv);
            atom_arg_getlong(&x->l_fade_in_time, 1, argc, argv);
            atom_arg_getlong(&x->l_fade_out_time, 2, argc, argv);
            break;
        default:
            object_post((t_object *)x, "received too many arguments");
    }
    
    addfades_set(x, tmp);
    x->l_outlet1 = bangout((t_object *)x);
    return x;
}

//void *split_new(t_symbol *s, long argc, t_atom *argv)
//{
//    t_split *x = (t_split *)object_alloc(s_split_class);
//
//    if (x) {
//        dsp_setup((t_pxobject *)x, 3);                // 3 inlets (input, low-value, high-value)
//        outlet_new((t_object *)x, "signal");        // last outlet: true/false
//        outlet_new((t_object *)x, "signal");        // middle outlet: values out-of-range
//        outlet_new((t_object *)x, "signal");        // first outlet: values w/in range
//
//        atom_arg_getfloat(&x->s_low, 0, argc, argv);    // get typed in args
//        atom_arg_getfloat(&x->s_high, 1, argc, argv);    // ...
//    }
//    return x;
//}


void addfades_free(t_addfades *x)
{
    dsp_free((t_pxobject *)x);
    object_free(x->l_buffer_reference);
}


t_max_err addfades_notify(t_addfades *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
    return buffer_ref_notify(x->l_buffer_reference, s, msg, sender, data);
}
