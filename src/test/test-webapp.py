import requests
import json
import time
import random
import sys, getopt
import signal

headers = {"Content-Type": "application/json;charset=utf8"}

exited = False
timeout = 0.2

def LoginTest(ip):
    url = ip + "/action/login"
    kk = '{"username": "admin", "password": "admin"}'
    response = json.loads(requests.post(url=url, headers=headers, data=kk, timeout=timeout).text)
    headers["Authorization"] = response["data"]["token"]

def OSDTest(host):
    url = host + "/overlay"
    kk = '{"src_id":0,"overlay_attr":[{"video":{"id":0,"width":2688,"height":1520},"time":{"enable":true,"format":11,"color":"0xFFFFFF","fontsize":96,"align":0,"inverse_enable":false,"color_inv":"0x000000","rect":[48,20,960,96]},"logo":{"enable":true,"align":3,"rect":[2400,1408,256,64]},"channel":{"enable":false,"text":"CHANNEL-*","color":"0xFFFFFF","fontsize":48,"align":1,"inverse_enable":false,"color_inv":"0x000000","rect":[2380,20,288,48]},"location":{"enable":false,"text":"LOCATION-*","color":"0xFFFFFF","fontsize":48,"align":2,"inverse_enable":false,"color_inv":"0x000000","rect":[20,1452,320,48]}},{"video":{"id":1,"width":720,"height":576},"time":{"enable":true,"format":11,"color":"0xFFFFFF","fontsize":32,"align":0,"inverse_enable":false,"color_inv":"0x000000","rect":[12,8,320,32]},"logo":{"enable":true,"align":3,"rect":[616,540,96,28]},"channel":{"enable":false,"text":"CHANNEL-*","color":"0xFFFFFF","fontsize":16,"align":1,"inverse_enable":false,"color_inv":"0x000000","rect":[568,8,144,16]},"location":{"enable":false,"text":"LOCATION-*","color":"0xFFFFFF","fontsize":16,"align":2,"inverse_enable":false,"color_inv":"0x000000","rect":[20,540,160,16]}}],"privacy_attr":{"enable":false,"type":1,"linewidth":2,"color":"0xFFFFFF","solid":false,"points":[[20,200],[420,200],[420,400],[20,400]],"sourceResolution":[2688,1520]}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def OSDTest1(host):
    url = host + "/overlay"
    kk = '{"src_id":0,"overlay_attr":[{"channel":{"align":1,"color":"0xFFFFFF","color_inv":"0x000000","enable":true,"fontsize":48,"inverse_enable":false,"rect":[1612,20,216,48],"text":"CHANNEL-*"},"location":{"align":2,"color":"0xFFFFFF","color_inv":"0x000000","enable":false,"fontsize":48,"inverse_enable":false,"rect":[20,1012,240,48],"text":"LOCATION-*"},"logo":{"align":3,"enable":true,"rect":[1632,968,256,64]},"time":{"align":0,"color":"0xFFFFFF","color_inv":"0x000000","enable":true,"fontsize":48,"format":11,"inverse_enable":false,"rect":[48,20,640,48]},"video":{"height":1080,"id":0,"width":1920}},{"channel":{"align":1,"color":"0xFFFFFF","color_inv":"0x000000","enable":false,"fontsize":16,"inverse_enable":false,"rect":[568,8,72,16],"text":"CHANNEL-*"},"location":{"align":2,"color":"0xFFFFFF","color_inv":"0x000000","enable":false,"fontsize":16,"inverse_enable":false,"rect":[20,540,80,16],"text":"LOCATION-*"},"logo":{"align":3,"enable":true,"rect":[616,540,96,28]},"time":{"align":0,"color":"0xFFFFFF","color_inv":"0x000000","enable":true,"fontsize":32,"format":11,"inverse_enable":false,"rect":[12,8,320,32]},"video":{"height":576,"id":1,"width":720}}],"privacy_attr":{"color":"0xFFFFFF","enable":true,"linewidth":2,"points":[[20,200],[420,200],[420,400],[20,400]],"solid":true,"sourceResolution":[2688,1520],"type":1}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def VideoTest(host):
    url = host + "/video"
    kk = '{"src_id":1,"video0":{"bit_rate":8192,"enable_res_chg":true,"enable_stream":true,"encoder_type":0,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"2688x1520","sht_stat_time":0},"video1":{"bit_rate":4096,"enable_res_chg":true,"enable_stream":true,"encoder_type":0,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"720x576","sht_stat_time":0}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text


def VideoTest2(host):
    url = host + "/video"
    kk = '{"src_id":0,"video0":{"bit_rate":8192,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"2688x1520","sht_stat_time":0},"video1":{"bit_rate":4096,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"720x576","sht_stat_time":0}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def VideoTest21(host):
    url = host + "/video"
    kk = '{"src_id":1,"video0":{"bit_rate":8192,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"2688x1520","sht_stat_time":0},"video1":{"bit_rate":4096,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"720x576","sht_stat_time":0}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def VideoTest3(host):
    url = host + "/video"
    kk = '{"src_id":0,"video0":{"bit_rate":8192,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"1920x1080","sht_stat_time":0},"video1":{"bit_rate":4096,"enable_res_chg":true,"enable_stream":true,"encoder_type":0,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":1,"resolution":"720x576","sht_stat_time":0}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def VideoTest31(host):
    url = host + "/video"
    kk = '{"src_id":1,"video0":{"bit_rate":8192,"enable_res_chg":true,"enable_stream":true,"encoder_type":2,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":0,"resolution":"1920x1080","sht_stat_time":0},"video1":{"bit_rate":4096,"enable_res_chg":true,"enable_stream":true,"encoder_type":0,"lt_max_bitrate":0,"lt_min_bitrate":0,"lt_stat_time":0,"max_iprop":40,"max_iqp":51,"max_qp":51,"min_iprop":10,"min_iqp":10,"min_qp":10,"rc_type":1,"resolution":"720x576","sht_stat_time":0}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text


def AITest(host):
    url = host + "/ai"
    kk = '{"src_id":0,"ai_attr":{"ai_enable":true,"detect_model":"hvcfp","detect_fps":10,"push_strategy":{"push_mode":"BEST_FRAME","push_interval":2000,"push_count":3,"push_same_frame":true},"detect_only":true,"facehuman":{"face_detect":{"enable":true,"draw_rect":true},"body_detect":{"enable":true,"draw_rect":true},"face_identify":{"enable":true}},"hvcfp":{"face_detect":{"enable":false,"draw_rect":false},"body_detect":{"enable":false,"draw_rect":false},"vechicle_detect":{"enable":false,"draw_rect":false},"cycle_detect":{"enable":false,"draw_rect":false},"plate_detect":{"enable":false,"draw_rect":false},"plate_identify":{"enable":false}},"events":{"motion_detect":{"enable":true,"threshold_y":20,"confidence":50},"occlusion_detect":{"enable":true,"threshold_y":100,"confidence":80},"scene_change_detect":{"enable":true,"threshold_y":60,"confidence":60}},"body_roi":{"enable":true,"min_width":167,"min_height":124,"mode":1}}}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def CameraTest(host):
    url = host + "/camera"
    kk = '{"src_id":0,"camera_attr":{"sns_work_mode":1,"rotation":0,"mirror":false,"flip":false,"framerate":30,"daynight":0,"nr_mode":1,"capture_enable":true,"switch_capture_enable":false,"switch_work_mode_enable":1,"switch_PN_mode_enable":1,"framerate_opts":[25,30],"switch_mirror_flip_enable":true,"switch_rotation_enable":true},"framerate_opts":[25,30]}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

def CameraTest25(host):
    url = host + "/camera"
    kk = '{"src_id":0,"camera_attr":{"sns_work_mode":1,"rotation":0,"mirror":false,"flip":false,"framerate":25,"daynight":0,"nr_mode":1,"capture_enable":true,"switch_capture_enable":false,"switch_work_mode_enable":1,"switch_PN_mode_enable":1,"framerate_opts":[25,30],"switch_mirror_flip_enable":true,"switch_rotation_enable":true},"framerate_opts":[25,30]}'
    requests.post(url=url, headers=headers, data=kk, timeout=timeout).text

aryMethod = [OSDTest, OSDTest1, VideoTest, VideoTest2, VideoTest21, VideoTest3, VideoTest31, AITest, CameraTest, CameraTest25]

def signal_handler(signal, frame):
    global exited
    exited = True

def main(argv):
    global sleep_time, exited
    try:
        opts, args = getopt.getopt(argv,"hi:t:",["ip=", "time="])
    except getopt.GetoptError:
        print('test.py -i <ip> -t <sleep time>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('test.py -i <ip> -t <sleep time>')
            sys.exit(0)
        elif opt in ("-i", "--ip"):
            ip = "http://" + arg + ":8080"
        elif opt in ("-t", "--time"):
            sleep_time = int(arg) * 0.001

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    host = ip + "/action/setting"
    try:
        LoginTest(ip)
    except requests.exceptions.Timeout:
        print("connect timeout")
        sys.exit(1)
    except requests.exceptions.RequestException as e:  # This is the correct syntax
        sys.exit(1)

    while exited:
        try:
            index = random.randint(0, len(aryMethod) - 1);
            aryMethod[index](host)
        except requests.exceptions.RequestException as e:  # This is the correct syntax
            pass
        time.sleep(sleep_time)

if __name__ == "__main__":
   main(sys.argv[1:])