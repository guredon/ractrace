#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ppb_raytrace.h"
#include "timer.h"	    // start_timer, stop_timer

//#define DEBUG               /* デバッグ処理の有効無効 */
#define OUTPUT     "out.ppm"  /* 結果画像のファイル名 */
#define IMG_WIDTH  (400)      /* 結果画像の幅 */
#define IMG_HEIGHT (400)      /* 結果画像の高さ */

#define THREAD_NUM 2 //追加

/* RGB値をセット*/
void set_color(rgb_t* buf, unsigned char r, unsigned char g, unsigned char b) {
    buf->r = r;
    buf->g = g;
    buf->b = b;
    return;
}

/* RGB値と単精度浮動小数点の乗算 */
void mult_color(rgb_t* b, rgb_t* a, float t) {
    b->r = (char)(t * a->r);
    b->g = (char)(t * a->g);
    b->b = (char)(t * a->b);
    return;
}

/* RGB値の加算 */
void add_color(rgb_t* c, rgb_t* a, rgb_t* b) {
    c->r = a->r + b->r;
    c->g = a->g + b->g;
    c->b = a->b + b->b;
    return;
}

/* 結果画像用領域へRGB値の描き込み */
void draw_pixel(rgb_t* buf, rgb_t* color) {
    buf->r = color->r;
    buf->g = color->g;
    buf->b = color->b;
    return;
}

/* ベクターへ要素をセット */
void set_vector(vector_t* v, float x, float y, float z) {
    v->x = x;	// ポインタvからvector_tのメンバxに, 値xを代入
    v->y = y;
    v->z = z;
    return;
}

/* ベクターの加算 */
void add_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
    return;
}

/* ベクターの各要素と単精度浮動小数点の内大きい方の値を求め、
   結果をベクターとして返す*/
void max_vector(vector_t* c, float n) {
    c->x = Max(c->x, n);
    c->y = Max(c->y, n);
    c->z = Max(c->z, n);
    return;    
}

/* ベクターの減算 */
void sub_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
    return;
}

/* ベクターの乗算 */
void mult_vector(vector_t* b, vector_t* a, float t) {
    b->x = t * a->x;
    b->y = t * a->y;
    b->z = t * a->z;
    return;
}

/* ベクターの標準化 */
void norm_vector(vector_t* a) {
    float d = sqrtf(Sq(a->x) + Sq(a->y) + Sq(a->z));
    a->x /= d;
    a->y /= d;
    a->z /= d;
    return;
}

/* ベクターの内積 */
float innerproduct_vector(vector_t* a, vector_t* b) {
    return (a->x*b->x + a->y*b->y + a->z*b->z);
}

/* 新しいシーンの生成 */
void new_scene(scene_data_t* scene) {
    /* �さ綢里凌А�中心、半径の設定 */
    set_color(&(scene->ball).color, SPHERE_COLOR);	    // SPHERE_COLOR : 描画領域に置く球体の色
    set_vector(&(scene->ball).center, SPHERE_CENTER);	    // SPHERE_CENTER : 中心座標
    (scene->ball).radius = SPHERE_RAD;			    // SPHERE_RAD : 半径

    /* �デ愀平А∋訶世虜舵検�光源の座標の設定 */
    set_color(&(scene->bgcolor), BG_COLOR);		    // BG_COLOR : 描画領域の背景色
    set_vector(&(scene->viewpoint), VIEW_POINT);	    // VIEW_POINT : 視点の座標
    set_vector(&(scene->light), LIGHT);			    // LIGHT : 光源の座標
    return;
}

/* 処理領域の設定 */
void set_workarea(workarea_t* warea, int sx, int ex, int sy, int ey) {
    warea->startx = sx;
    warea->starty = sy;
    warea->endx = ex;
    warea->endy = ey;
    return;
}


//*****************************************************************************
// 関数名 : init_trace_thread
// 引数   : thread_num:スレッド数, targ*, warea*, img*, scene*, w, h
// 戻り値 : なし
// 概要   : 作業用スレッドがレイトレーシングの計算をするために使用する各種パラメータの設定
//*****************************************************************************
void init_trace_thread (int thread_num, thread_arg_t* targ, workarea_t* warea, image_t* img, scene_data_t* scene, int w, int h) {
	int i;

	for (i = 0; i < thread_num; ++i) {
	    /* �� スレッドの計算範囲の指定 */
	    set_workarea(&warea[i], 0, w, i*h/thread_num, (i+1)*h/thread_num);	// 各スレッドの計算範囲
	    /* ��レイトレース計算用パラメータの設定 */
	    targ[i].img = img;
	    targ[i].scene = scene;
	    targ[i].warea = warea[i];
	}
	return;
}


//*****************************************************************************
// 関数名 : start_trace_thread
// 引数   : thread_num:スレッド数, tid, targ
// 戻り値 : なし
// 概要   : init_trace_thread で設定されたパラメータに基いてスレッドを作成
//*****************************************************************************
void start_trace_thread (int thread_num, pthread_t* tid, thread_arg_t* targ) {
	int i;
	for(i = 0; i < thread_num; ++i)
		ptread_create(&tid[i], NULL, (void*)trace_ray_thread, (void*)&targ[i]);
	return;
}


//*****************************************************************************
// 関数名 : trace_ray_thread 
// 引数   : thread_arg_t構造体に含まれるパラメータ:arg
// 戻り値 : なし
// 概要   : trace_ray関数の呼び出し
//*****************************************************************************
void trace_ray_thread (thread_arg_t* arg) {
	image_t* img = arg->img;
	scene_data_t* scene = arg->scene;
	workarea_t* warea = &arg->warea;

	trace_ray(img, scene, warea);
	return;
}


//*****************************************************************************
// 関数名 : wait_trace_thread
// 引数   : thread_num:スレッド数, tid
// 戻り値 : なし
// 概要   : スレッド終了待機のための関数を呼ぶ. この関数の処理が終わればレイトレーシングの計算は完了
//*****************************************************************************
void wait_trace_thread (int thread_num, pthread_t* tid){
	int i;
	for(i = 0; i < thread_num; ++i) {
		pthread_join(tid[i], NULL);
	}
	return;
}


//*****************************************************************************
// 関数名 : intersect
// 引数   : ball:球体, viewpoint:視点, view:視線
// 戻り値 : t（視線と球体との交点を求めるために必要となる係数）
// 概要   : 視線が球体と交わるかどうかについての判定
//*****************************************************************************
float intersect(sphere_t* ball, vector_t* viewpoint, vector_t* view) { 
    float t;
    vector_t vtmp;
    /* ��判別式の計算 */
    sub_vector(&vtmp, viewpoint, &ball->center);    // 交差判定の二次式
    float b = innerproduct_vector(view, &vtmp);
    float c = innerproduct_vector(&vtmp, &vtmp) - Sq(ball->radius);
    float d = Sq(b) - c;
    /* ��交差状況の判定 */
    if(d < 0) return INFINITY;	    // 0より小さい = 交差しないので無限大を返す
    float det = sqrtf(d);
    t = -b - det;
    if(t < 0.05f) t = -b + det;	    // 判別式の値が小さい方を選択する
    if(t < 0) return INFINITY;
    return t;			    // 返り値t
}


//*****************************************************************************
// 関数名 : shading
// 引数   : view:視線, light:光線, n:法線, color:球体の色, s:光の強さを表す係数
// 戻り値 : 
// 概要   : 交点の色の明暗を計算する
//*****************************************************************************

/* シェーディング(フォンモデル) */
rgb_t shading(vector_t* view, vector_t* light, vector_t* n, rgb_t* color, float s) {
    /* ��シェーディングパラメータ設定 */
    float kd = 0.7f, ks = 0.7f, ke = 0.3f;	// kd:拡散反射光, ks:鏡面反射光, ke:環境光の係数
    /* ��色の明暗を計算 */
    rgb_t white;
    set_color(&white, 255, 255, 255);		// 環境光の色に白を設定
    rgb_t c, c0, c1;
    float ln = innerproduct_vector(light, n);
    float lv = innerproduct_vector(light, view);
    float nv = innerproduct_vector(n, view);
    float cos_alpha = Max((-1.0f * ln), 0.0f);
    float cos_bata  = Max((2.0f*ln * nv - lv),0.0f);
    float cos_bata_pow20;
    Pow(cos_bata_pow20, cos_bata, 20);			    // 色の質感に影響. 大きくなるほど金属的になる
    mult_color(&c0, color,  (s * kd * cos_alpha + ke));
    mult_color(&c1, &white, (s * ks * cos_bata_pow20));
    add_color(&c, &c0, &c1);
    return c;
}

/* レイトレースの計算 */
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

	/* �Ε譽ぅ肇譟璽垢侶彁使楼� */
    for (y = warea->starty; y < warea->endy; ++y) {
        viewpoint.y = y*2.0f/h-1.0f;
        for (x = warea->startx; x < warea->endx; ++x) {
            /* �Щ訐�ベクトルを求める */
            viewpoint.x = x*2.0f/w-1.0f;	    
            float dv = sqrtf(Sq(viewpoint.x)+Sq(viewpoint.y)+Sq(viewpoint.z));
            set_vector(&view, x, y, -dv);   // &view:vector_tのアドレスを入れる, vector_tのx, y, z
            sub_vector(&view, &view, &viewpoint);
            norm_vector(&view);	    // 求めた視線ベクトルviewは、norm_vector関数によって標準化
            int index = y*w+x;
            /* �┯鮑紅縦蟒萢� ： 視線ベクトルが球体と交わるかどうか*/
#ifdef DEBUG
    start_timer(&t0);
#endif
            float t = intersect(&ball, &viewpoint, &view);  // intersect : 交わる場合は交点の係数tを交わらない場合は∞を返す
							    // ∞を返した場合、そこは背景である
#ifdef DEBUG
	    intersect_time += stop_timer(&t0);
#endif
            if (t < INFINITY) {				    // 交点と交わる（t）だった場合
                /* ��シェーディング処理に使用するパラメータの計算 */
                vector_t p, tv, n, L;
		// 視線ベクトルと球体との交点における法線ベクトルを求める
                mult_vector(&tv, &view, t);	    // 係数 t を用いて交点ベクトル p を求める
                add_vector(&p, &viewpoint, &tv);    // 交点ベクトル p と球体の中心ベクトルから法線ベクトル n を求める
                sub_vector(&n, &p, &ball.center);
                norm_vector(&n);		    // 法線ベクトル n の標準化
		// 光源から発射される光源ベクトルを求める
                sub_vector(&L, &p, &light);	    // 交点ベクトル p と光源ベクトルから光線ベクトル L を求める
                norm_vector(&L);
                /* ��シェーディング処理 */
#ifdef DEBUG
                start_timer(&t1);
#endif
                color = shading(&view, &L, &n, &ball.color, 0.5f);	// シェーディング処理
#ifdef DEBUG
                shading_time += stop_timer(&t1);
#endif
                draw_pixel(&img->buf[index], &color);
            } else {
		/* ��背景色の設定 */
                draw_pixel(&img->buf[index], &bgcolor);	    // 交点が背景（∞）だった場合、交点の座標に対して背景色を設定
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

    thread_arg_t targ[THREAD_NUM]; //追加
    pthread_t tid[THREAD_NUM]; //追加

    thread_arg_t arg[THREAD_NUM]; //追加

    int w = IMG_WIDTH;
    int h = IMG_HEIGHT;

    new_image(&img, w, h);		// 画像を確保するためのメモリ領域を確保
    new_scene(&scene);			// レイトレーシング計算のためのシーンの初期化

    start_timer(&t);

    /* �．好譽奪百慙⊇萢� */
    init_trace_thread(THREAD_NUM, targ, warea, &img, &scene, w, h);	// 2つのスレッドの作業領域とパラメータの設定
    start_trace_thread(THREAD_NUM, tid, targ);				// スレッド生成, 処理の開始
    wait_trace_thread(THREAD_NUM, tid);					// 作業スレッド終了の待機
    trace_ray_thread(arg); // 追加

    time = stop_timer(&t);
    print_timer(time);
    write_ppm(&img, OUTPUT);		// OUTPUTで指定されている"out.ppm"というファイル名で出力される
    delete_image(&img);

    return 0;
}

