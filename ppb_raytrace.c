#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ppb_raytrace.h"
#include "timer.h"	    // start_timer, stop_timer

//#define DEBUG               /* ¥Ç¥Ð¥Ã¥°½èÍý¤ÎÍ­¸úÌµ¸ú */
#define OUTPUT     "out.ppm"  /* ·ë²Ì²èÁü¤Î¥Õ¥¡¥¤¥ëÌ¾ */
#define IMG_WIDTH  (400)      /* ·ë²Ì²èÁü¤ÎÉý */
#define IMG_HEIGHT (400)      /* ·ë²Ì²èÁü¤Î¹â¤µ */

#define THREAD_NUM 2 //ÄÉ²Ã

/* RGBÃÍ¤ò¥»¥Ã¥È*/
void set_color(rgb_t* buf, unsigned char r, unsigned char g, unsigned char b) {
    buf->r = r;
    buf->g = g;
    buf->b = b;
    return;
}

/* RGBÃÍ¤ÈÃ±ÀºÅÙÉâÆ°¾®¿ôÅÀ¤Î¾è»» */
void mult_color(rgb_t* b, rgb_t* a, float t) {
    b->r = (char)(t * a->r);
    b->g = (char)(t * a->g);
    b->b = (char)(t * a->b);
    return;
}

/* RGBÃÍ¤Î²Ã»» */
void add_color(rgb_t* c, rgb_t* a, rgb_t* b) {
    c->r = a->r + b->r;
    c->g = a->g + b->g;
    c->b = a->b + b->b;
    return;
}

/* ·ë²Ì²èÁüÍÑÎÎ°è¤ØRGBÃÍ¤ÎÉÁ¤­¹þ¤ß */
void draw_pixel(rgb_t* buf, rgb_t* color) {
    buf->r = color->r;
    buf->g = color->g;
    buf->b = color->b;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤ØÍ×ÁÇ¤ò¥»¥Ã¥È */
void set_vector(vector_t* v, float x, float y, float z) {
    v->x = x;	// ¥Ý¥¤¥ó¥¿v¤«¤évector_t¤Î¥á¥ó¥Ðx¤Ë, ÃÍx¤òÂåÆþ
    v->y = y;
    v->z = z;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤Î²Ã»» */
void add_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤Î³ÆÍ×ÁÇ¤ÈÃ±ÀºÅÙÉâÆ°¾®¿ôÅÀ¤ÎÆâÂç¤­¤¤Êý¤ÎÃÍ¤òµá¤á¡¢
   ·ë²Ì¤ò¥Ù¥¯¥¿¡¼¤È¤·¤ÆÊÖ¤¹*/
void max_vector(vector_t* c, float n) {
    c->x = Max(c->x, n);
    c->y = Max(c->y, n);
    c->z = Max(c->z, n);
    return;    
}

/* ¥Ù¥¯¥¿¡¼¤Î¸º»» */
void sub_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤Î¾è»» */
void mult_vector(vector_t* b, vector_t* a, float t) {
    b->x = t * a->x;
    b->y = t * a->y;
    b->z = t * a->z;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤ÎÉ¸½à²½ */
void norm_vector(vector_t* a) {
    float d = sqrtf(Sq(a->x) + Sq(a->y) + Sq(a->z));
    a->x /= d;
    a->y /= d;
    a->z /= d;
    return;
}

/* ¥Ù¥¯¥¿¡¼¤ÎÆâÀÑ */
float innerproduct_vector(vector_t* a, vector_t* b) {
    return (a->x*b->x + a->y*b->y + a->z*b->z);
}

/* ¿·¤·¤¤¥·¡¼¥ó¤ÎÀ¸À® */
void new_scene(scene_data_t* scene) {
    /* ­¤µåÂÎ¤Î¿§¡¢Ãæ¿´¡¢È¾·Â¤ÎÀßÄê */
    set_color(&(scene->ball).color, SPHERE_COLOR);	    // SPHERE_COLOR : ÉÁ²èÎÎ°è¤ËÃÖ¤¯µåÂÎ¤Î¿§
    set_vector(&(scene->ball).center, SPHERE_CENTER);	    // SPHERE_CENTER : Ãæ¿´ºÂÉ¸
    (scene->ball).radius = SPHERE_RAD;			    // SPHERE_RAD : È¾·Â

    /* ­¥ÇØ·Ê¿§¡¢»ëÅÀ¤ÎºÂÉ¸¡¢¸÷¸»¤ÎºÂÉ¸¤ÎÀßÄê */
    set_color(&(scene->bgcolor), BG_COLOR);		    // BG_COLOR : ÉÁ²èÎÎ°è¤ÎÇØ·Ê¿§
    set_vector(&(scene->viewpoint), VIEW_POINT);	    // VIEW_POINT : »ëÅÀ¤ÎºÂÉ¸
    set_vector(&(scene->light), LIGHT);			    // LIGHT : ¸÷¸»¤ÎºÂÉ¸
    return;
}

/* ½èÍýÎÎ°è¤ÎÀßÄê */
void set_workarea(workarea_t* warea, int sx, int ex, int sy, int ey) {
    warea->startx = sx;
    warea->starty = sy;
    warea->endx = ex;
    warea->endy = ey;
    return;
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : init_trace_thread
// °ú¿ô   : thread_num:¥¹¥ì¥Ã¥É¿ô, targ*, warea*, img*, scene*, w, h
// Ìá¤êÃÍ : ¤Ê¤·
// ³µÍ×   : ºî¶ÈÍÑ¥¹¥ì¥Ã¥É¤¬¥ì¥¤¥È¥ì¡¼¥·¥ó¥°¤Î·×»»¤ò¤¹¤ë¤¿¤á¤Ë»ÈÍÑ¤¹¤ë³Æ¼ï¥Ñ¥é¥á¡¼¥¿¤ÎÀßÄê
//*****************************************************************************
void init_trace_thread (int thread_num, thread_arg_t* targ, workarea_t* warea, image_t* img, scene_data_t* scene, int w, int h) {
	int i;

	for (i = 0; i < thread_num; ++i) {
	    /* ­¢ ¥¹¥ì¥Ã¥É¤Î·×»»ÈÏ°Ï¤Î»ØÄê */
	    set_workarea(&warea[i], 0, w, i*h/thread_num, (i+1)*h/thread_num);	// ³Æ¥¹¥ì¥Ã¥É¤Î·×»»ÈÏ°Ï
	    /* ­£¥ì¥¤¥È¥ì¡¼¥¹·×»»ÍÑ¥Ñ¥é¥á¡¼¥¿¤ÎÀßÄê */
	    targ[i].img = img;
	    targ[i].scene = scene;
	    targ[i].warea = warea[i];
	}
	return;
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : start_trace_thread
// °ú¿ô   : thread_num:¥¹¥ì¥Ã¥É¿ô, tid, targ
// Ìá¤êÃÍ : ¤Ê¤·
// ³µÍ×   : init_trace_thread ¤ÇÀßÄê¤µ¤ì¤¿¥Ñ¥é¥á¡¼¥¿¤Ë´ð¤¤¤Æ¥¹¥ì¥Ã¥É¤òºîÀ®
//*****************************************************************************
void start_trace_thread (int thread_num, pthread_t* tid, thread_arg_t* targ) {
	int i;
	for(i = 0; i < thread_num; ++i)
		ptread_create(&tid[i], NULL, (void*)trace_ray_thread, (void*)&targ[i]);
	return;
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : trace_ray_thread 
// °ú¿ô   : thread_arg_t¹½Â¤ÂÎ¤Ë´Þ¤Þ¤ì¤ë¥Ñ¥é¥á¡¼¥¿:arg
// Ìá¤êÃÍ : ¤Ê¤·
// ³µÍ×   : trace_ray´Ø¿ô¤Î¸Æ¤Ó½Ð¤·
//*****************************************************************************
void trace_ray_thread (thread_arg_t* arg) {
	image_t* img = arg->img;
	scene_data_t* scene = arg->scene;
	workarea_t* warea = &arg->warea;

	trace_ray(img, scene, warea);
	return;
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : wait_trace_thread
// °ú¿ô   : thread_num:¥¹¥ì¥Ã¥É¿ô, tid
// Ìá¤êÃÍ : ¤Ê¤·
// ³µÍ×   : ¥¹¥ì¥Ã¥É½ªÎ»ÂÔµ¡¤Î¤¿¤á¤Î´Ø¿ô¤ò¸Æ¤Ö. ¤³¤Î´Ø¿ô¤Î½èÍý¤¬½ª¤ï¤ì¤Ð¥ì¥¤¥È¥ì¡¼¥·¥ó¥°¤Î·×»»¤Ï´°Î»
//*****************************************************************************
void wait_trace_thread (int thread_num, pthread_t* tid){
	int i;
	for(i = 0; i < thread_num; ++i) {
		pthread_join(tid[i], NULL);
	}
	return;
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : intersect
// °ú¿ô   : ball:µåÂÎ, viewpoint:»ëÅÀ, view:»ëÀþ
// Ìá¤êÃÍ : t¡Ê»ëÀþ¤ÈµåÂÎ¤È¤Î¸òÅÀ¤òµá¤á¤ë¤¿¤á¤ËÉ¬Í×¤È¤Ê¤ë·¸¿ô¡Ë
// ³µÍ×   : »ëÀþ¤¬µåÂÎ¤È¸ò¤ï¤ë¤«¤É¤¦¤«¤Ë¤Ä¤¤¤Æ¤ÎÈ½Äê
//*****************************************************************************
float intersect(sphere_t* ball, vector_t* viewpoint, vector_t* view) { 
    float t;
    vector_t vtmp;
    /* ­¬È½ÊÌ¼°¤Î·×»» */
    sub_vector(&vtmp, viewpoint, &ball->center);    // ¸òº¹È½Äê¤ÎÆó¼¡¼°
    float b = innerproduct_vector(view, &vtmp);
    float c = innerproduct_vector(&vtmp, &vtmp) - Sq(ball->radius);
    float d = Sq(b) - c;
    /* ­­¸òº¹¾õ¶·¤ÎÈ½Äê */
    if(d < 0) return INFINITY;	    // 0¤è¤ê¾®¤µ¤¤ = ¸òº¹¤·¤Ê¤¤¤Î¤ÇÌµ¸ÂÂç¤òÊÖ¤¹
    float det = sqrtf(d);
    t = -b - det;
    if(t < 0.05f) t = -b + det;	    // È½ÊÌ¼°¤ÎÃÍ¤¬¾®¤µ¤¤Êý¤òÁªÂò¤¹¤ë
    if(t < 0) return INFINITY;
    return t;			    // ÊÖ¤êÃÍt
}


//*****************************************************************************
// ´Ø¿ôÌ¾ : shading
// °ú¿ô   : view:»ëÀþ, light:¸÷Àþ, n:Ë¡Àþ, color:µåÂÎ¤Î¿§, s:¸÷¤Î¶¯¤µ¤òÉ½¤¹·¸¿ô
// Ìá¤êÃÍ : 
// ³µÍ×   : ¸òÅÀ¤Î¿§¤ÎÌÀ°Å¤ò·×»»¤¹¤ë
//*****************************************************************************

/* ¥·¥§¡¼¥Ç¥£¥ó¥°(¥Õ¥©¥ó¥â¥Ç¥ë) */
rgb_t shading(vector_t* view, vector_t* light, vector_t* n, rgb_t* color, float s) {
    /* ­®¥·¥§¡¼¥Ç¥£¥ó¥°¥Ñ¥é¥á¡¼¥¿ÀßÄê */
    float kd = 0.7f, ks = 0.7f, ke = 0.3f;	// kd:³È»¶È¿¼Í¸÷, ks:¶ÀÌÌÈ¿¼Í¸÷, ke:´Ä¶­¸÷¤Î·¸¿ô
    /* ­¯¿§¤ÎÌÀ°Å¤ò·×»» */
    rgb_t white;
    set_color(&white, 255, 255, 255);		// ´Ä¶­¸÷¤Î¿§¤ËÇò¤òÀßÄê
    rgb_t c, c0, c1;
    float ln = innerproduct_vector(light, n);
    float lv = innerproduct_vector(light, view);
    float nv = innerproduct_vector(n, view);
    float cos_alpha = Max((-1.0f * ln), 0.0f);
    float cos_bata  = Max((2.0f*ln * nv - lv),0.0f);
    float cos_bata_pow20;
    Pow(cos_bata_pow20, cos_bata, 20);			    // ¿§¤Î¼Á´¶¤Ë±Æ¶Á. Âç¤­¤¯¤Ê¤ë¤Û¤É¶âÂ°Åª¤Ë¤Ê¤ë
    mult_color(&c0, color,  (s * kd * cos_alpha + ke));
    mult_color(&c1, &white, (s * ks * cos_bata_pow20));
    add_color(&c, &c0, &c1);
    return c;
}

/* ¥ì¥¤¥È¥ì¡¼¥¹¤Î·×»» */
void trace_ray(image_t* img, scene_data_t* scene, workarea_t* warea) {
    unsigned int t0 = 0, intersect_time = 0, t1 = 0, shading_time = 0;
    int x, y;
    rgb_t color;

    int w = img->width;
    int h = img->height;
    vector_t view = scene->view;
    vector_t viewpoint = scene->viewpoint;
    vector_t light = scene->light;
    sphere_t ball = scene->ball;
    rgb_t bgcolor = scene->bgcolor;

	/* ­¦¥ì¥¤¥È¥ì¡¼¥¹¤Î·×»»ÈÏ°Ï */
    for (y = warea->starty; y < warea->endy; ++y) {
        viewpoint.y = y*2.0f/h-1.0f;
        for (x = warea->startx; x < warea->endx; ++x) {
            /* ­§»ëÀþ¥Ù¥¯¥È¥ë¤òµá¤á¤ë */
            viewpoint.x = x*2.0f/w-1.0f;	    
            float dv = sqrtf(Sq(viewpoint.x)+Sq(viewpoint.y)+Sq(viewpoint.z));
            set_vector(&view, x, y, -dv);   // &view:vector_t¤Î¥¢¥É¥ì¥¹¤òÆþ¤ì¤ë, vector_t¤Îx, y, z
            sub_vector(&view, &view, &viewpoint);
            norm_vector(&view);	    // µá¤á¤¿»ëÀþ¥Ù¥¯¥È¥ëview¤Ï¡¢norm_vector´Ø¿ô¤Ë¤è¤Ã¤ÆÉ¸½à²½
            int index = y*w+x;
            /* ­¨¸òº¹È½Äê½èÍý ¡§ »ëÀþ¥Ù¥¯¥È¥ë¤¬µåÂÎ¤È¸ò¤ï¤ë¤«¤É¤¦¤«*/
#ifdef DEBUG
    start_timer(&t0);
#endif
            float t = intersect(&ball, &viewpoint, &view);  // intersect : ¸ò¤ï¤ë¾ì¹ç¤Ï¸òÅÀ¤Î·¸¿ôt¤ò¸ò¤ï¤é¤Ê¤¤¾ì¹ç¤Ï¡ç¤òÊÖ¤¹
							    // ¡ç¤òÊÖ¤·¤¿¾ì¹ç¡¢¤½¤³¤ÏÇØ·Ê¤Ç¤¢¤ë
#ifdef DEBUG
	    intersect_time += stop_timer(&t0);
#endif
            if (t < INFINITY) {				    // ¸òÅÀ¤È¸ò¤ï¤ë¡Êt¡Ë¤À¤Ã¤¿¾ì¹ç
                /* ­ª¥·¥§¡¼¥Ç¥£¥ó¥°½èÍý¤Ë»ÈÍÑ¤¹¤ë¥Ñ¥é¥á¡¼¥¿¤Î·×»» */
                vector_t p, tv, n, L;
		// »ëÀþ¥Ù¥¯¥È¥ë¤ÈµåÂÎ¤È¤Î¸òÅÀ¤Ë¤ª¤±¤ëË¡Àþ¥Ù¥¯¥È¥ë¤òµá¤á¤ë
                mult_vector(&tv, &view, t);	    // ·¸¿ô t ¤òÍÑ¤¤¤Æ¸òÅÀ¥Ù¥¯¥È¥ë p ¤òµá¤á¤ë
                add_vector(&p, &viewpoint, &tv);    // ¸òÅÀ¥Ù¥¯¥È¥ë p ¤ÈµåÂÎ¤ÎÃæ¿´¥Ù¥¯¥È¥ë¤«¤éË¡Àþ¥Ù¥¯¥È¥ë n ¤òµá¤á¤ë
                sub_vector(&n, &p, &ball.center);
                norm_vector(&n);		    // Ë¡Àþ¥Ù¥¯¥È¥ë n ¤ÎÉ¸½à²½
		// ¸÷¸»¤«¤éÈ¯¼Í¤µ¤ì¤ë¸÷¸»¥Ù¥¯¥È¥ë¤òµá¤á¤ë
                sub_vector(&L, &p, &light);	    // ¸òÅÀ¥Ù¥¯¥È¥ë p ¤È¸÷¸»¥Ù¥¯¥È¥ë¤«¤é¸÷Àþ¥Ù¥¯¥È¥ë L ¤òµá¤á¤ë
                norm_vector(&L);
                /* ­«¥·¥§¡¼¥Ç¥£¥ó¥°½èÍý */
#ifdef DEBUG
                start_timer(&t1);
#endif
                color = shading(&view, &L, &n, &ball.color, 0.5f);	// ¥·¥§¡¼¥Ç¥£¥ó¥°½èÍý
#ifdef DEBUG
                shading_time += stop_timer(&t1);
#endif
                draw_pixel(&img->buf[index], &color);
            } else {
		/* ­©ÇØ·Ê¿§¤ÎÀßÄê */
                draw_pixel(&img->buf[index], &bgcolor);	    // ¸òÅÀ¤¬ÇØ·Ê¡Ê¡ç¡Ë¤À¤Ã¤¿¾ì¹ç¡¢¸òÅÀ¤ÎºÂÉ¸¤ËÂÐ¤·¤ÆÇØ·Ê¿§¤òÀßÄê
            }
        }
    }

#ifdef DEBUG
    print_timer(intersect_time);
    print_timer(shading_time);
#endif

    return;
}

int main(int argc, char* argv[]) {
    unsigned int t, time;
    image_t img;
    scene_data_t scene;
    workarea_t warea[THREAD_NUM];

    thread_arg_t targ[THREAD_NUM]; //ÄÉ²Ã
    pthread_t tid[THREAD_NUM]; //ÄÉ²Ã

    thread_arg_t arg[THREAD_NUM]; //ÄÉ²Ã

    int w = IMG_WIDTH;
    int h = IMG_HEIGHT;

    new_image(&img, w, h);		// ²èÁü¤ò³ÎÊÝ¤¹¤ë¤¿¤á¤Î¥á¥â¥êÎÎ°è¤ò³ÎÊÝ
    new_scene(&scene);			// ¥ì¥¤¥È¥ì¡¼¥·¥ó¥°·×»»¤Î¤¿¤á¤Î¥·¡¼¥ó¤Î½é´ü²½

    start_timer(&t);

    /* ­¡¥¹¥ì¥Ã¥É´ØÏ¢½èÍý */
    init_trace_thread(THREAD_NUM, targ, warea, &img, &scene, w, h);	// 2¤Ä¤Î¥¹¥ì¥Ã¥É¤Îºî¶ÈÎÎ°è¤È¥Ñ¥é¥á¡¼¥¿¤ÎÀßÄê
    start_trace_thread(THREAD_NUM, tid, targ);				// ¥¹¥ì¥Ã¥ÉÀ¸À®, ½èÍý¤Î³«»Ï
    wait_trace_thread(THREAD_NUM, tid);					// ºî¶È¥¹¥ì¥Ã¥É½ªÎ»¤ÎÂÔµ¡
    trace_ray_thread(arg); // ÄÉ²Ã

    time = stop_timer(&t);
    print_timer(time);
    write_ppm(&img, OUTPUT);		// OUTPUT¤Ç»ØÄê¤µ¤ì¤Æ¤¤¤ë"out.ppm"¤È¤¤¤¦¥Õ¥¡¥¤¥ëÌ¾¤Ç½ÐÎÏ¤µ¤ì¤ë
    delete_image(&img);

    return 0;
}

