/**
 @file
 addfades~ - SDK example of an object which accesses an MSP buffer~

 @ingroup    examples
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_common.h" // contains CLAMP macro
#include "z_dsp.h"
#include "ext_buffer.h"


typedef struct _addfades {
    t_pxobject l_obj;
    t_buffer_ref *l_buffer_reference;
//    long l_chan;
    int l_fadeInTime;
    int l_fadeOutTime;
} t_addfades;


//void addfades_perform64(t_addfades *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
//void addfades_dsp64(t_addfades *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void addfades_set(t_addfades *x, t_symbol *s);
void *addfades_new(t_symbol *s, long chan);
void addfades_free(t_addfades *x);
t_max_err addfades_notify(t_addfades *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void addfades_in1(t_addfades *x, long n);
void addfades_bang(t_addfades *x);
void addfades_assist(t_addfades *x, void *b, long m, long a, char *s);
void addfades_dblclick(t_addfades *x);


static t_class *addfades_class;


C74_EXPORT void ext_main(void *r)
{
    t_class *c = class_new("addfades~", (method)addfades_new, (method)addfades_free, sizeof(t_addfades), 0L, A_SYM, A_DEFLONG, 0);

//    class_addmethod(c, (method)addfades_dsp64, "dsp64", A_CANT, 0);
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


//void addfades_perform64(t_addfades *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
//{
//    t_double    *in = ins[0];
//    t_double    *out = outs[0];
//    int            n = sampleframes;
//    t_float        *tab;
//    double        temp;
//    double        f;
//    long        index, chan, frames, nc;
//    t_buffer_obj    *buffer = buffer_ref_getobject(x->l_buffer_reference);
//
//    tab = buffer_locksamples(buffer);
//    if (!tab)
//        goto zero;
//
//    frames = buffer_getframecount(buffer);
//    nc = buffer_getchannelcount(buffer);
//    chan = MIN(x->l_chan, nc);
////    while (n--) {
////        temp = *in++;
////        f = temp + 0.5;
////        index = f;
////        if (index < 0)
////            index = 0;
////        else if (index >= frames)
////            index = frames - 1;
//        for (int i = 0; i < frames; i++) {
//            if (nc > 1)
//                index = index * nc + chan;
//    //        *out++ = tab[index];
//            tab[index] *= 0.25;
//        }
////    }
//    buffer_unlocksamples(buffer);
//    return;
//zero:
//    while (n--)
//        *out++ = 0.0;
//}

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
    double          temp;
    double          f;
    long            chan, frames, nc;
    t_buffer_obj    *buffer = buffer_ref_getobject(x->l_buffer_reference);

    t_buffer_info *info = malloc(sizeof(t_buffer_info));
    buffer_getinfo(buffer, info);
    post("%li\n", info->b_modtime);
    free(info);
    tab = buffer_locksamples(buffer);
    if (!tab)
        goto zero;
    frames = buffer_getframecount(buffer);
    nc = buffer_getchannelcount(buffer);
    post("before: sample at %li is %f\n", 10000, tab[10000]);
    for (long i = 0; i < frames; i++)
    {
//        if (nc > 1)
//            i = i * nc + chan;
        for (int c = 0; c < nc; c++)
        {
            tab[i * nc + c] *= 0.25;
        }
    }
    post("after: sample at %li is %f\n", 10000, tab[10000]);
    buffer_setdirty(buffer);
    buffer_unlocksamples(buffer);

    
    return;
zero:
    post("buffer is empty");
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

void *addfades_new(t_symbol *s, long chan)
{
    t_addfades *x = object_alloc(addfades_class);
    dsp_setup((t_pxobject *)x, 1);
    intin((t_object *)x,1);
    outlet_new((t_object *)x, "signal");
    addfades_set(x, s);
    addfades_in1(x,chan);
    return (x);
}


void addfades_free(t_addfades *x)
{
    dsp_free((t_pxobject *)x);
    object_free(x->l_buffer_reference);
}


t_max_err addfades_notify(t_addfades *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
    return buffer_ref_notify(x->l_buffer_reference, s, msg, sender, data);
}
