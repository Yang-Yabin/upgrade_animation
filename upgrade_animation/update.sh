#!/bin/sh

UPGRADEFILE="/userdata/upgradefile"
LOGFILE="/userdata/update_log"
ZIP_FILE="/userdata/upgradefile_tmp/BulkUpgrade.zip"

readonly log_file="/userdata/upgrade.log"
readonly upgrade_check_script="upgrade_check.sh"
readonly recovery_cmd="boot-recovery"
readonly recovery_log_file="/userdata/recovery/last_log"
readonly upgrade_check="${UPGRADEFILE}/${upgrade_check_script}"
readonly version_file_name="version"
ui_show_step_value='100000'

# 升级失败达到指定次数时退出 recovery 模式
readonly count_filepath='/userdata/upgrade.count'
readonly count_max='3'
readonly exit_disabled_str='disabled'

readonly oem_path='/userdata/oem'
readonly fake_zip='/userdata/upgradefile_tmp/fake.zip'

unset_recovery() {
    dd 'bs=1' 'seek=16384' 'of=/dev/block/by-name/misc' 'if=/dev/zero' 'count=1088' 2>/dev/null
}

set_recovery() {
    unset_recovery
    echo -n "${recovery_cmd}" | dd 'bs=1' 'seek=16384' 'of=/dev/block/by-name/misc' 'count=1088' 2>/dev/null
}

is_recovery() {
    ! df | grep -qE '[[:space:]]/oem$'
}

log() {
    local now_date="$(date '+%Y-%m-%d %X')"
    echo "${now_date} \$ $*" >> ${LOGFILE}
    echo "${now_date} \$ $*"
}

ui_show() {
    bin="/userdata/upgrade_ui"

    [ ! -f $bin ] && return 1

    killall $(basename "${bin}")

    case $1 in
        ok|error)
            $bin $1
            sleep 1
            ;;
        file)
            $bin $1
            ;;
        step)
            $bin $* &
            ;;
        *)
            return 1
            ;;
    esac
}

get_ui_show_step_value() {
    log "Start get_ui_show_step_value()..."

    unzip -oq "${ZIP_FILE}" "${version_file_name}" -d "${UPGRADEFILE}"

    local product_name=$(sed -n '/^# product/{s/^# product *//g;p}' "${UPGRADEFILE}/${version_file_name}")

    case "${product_name}" in
        'SVP3090'*|'SVP3060'*|'DP22'*)
            if check_space_is_enough 'zip_oem.img' '/userdata';then
                ui_show_step_value='30000'
            else
                ui_show_step_value='50000'
            fi
            ;;
        'DP32'*|'DP42'*)
            if check_space_is_enough 'zip_oem.img' '/userdata';then
                ui_show_step_value='8400'
            else
                ui_show_step_value='28000'
            fi
            ;;
        *)
            log "No step value specified for this product[${product_name}] !"
            ;;
    esac
    return 0
}

# 适配 EMU 升级结果上报
emu_report() {
    local dirname="$(dirname "${recovery_log_file}")"
    if ! [ -d "${dirname}" ];then
        # 如果有与目录同名的文件, mkdir 将失败
        mkdir -p "${dirname}"
        if ! [ -d "${dirname}" ];then
            log "mkdir -p '${dirname}' failed !"
            return 1
        fi
    fi
    case "$1" in
        'ok')
            echo 'success' > "${recovery_log_file}"
            ;;
        'error')
            echo 'failure' > "${recovery_log_file}"
            ;;
        *)
            log "emu_report unknown args: [$*]"
            ;;
    esac
}

check_count() {
    # 升级失败超过 count_max 次时退出 recovery 模式
    local count='1'
    if [ -f "${count_filepath}" ];then
        count="$(cat "${count_filepath}")"
        if [ "${count}" = "${exit_disabled_str}" ];then
            log "Can't exit recovery: disabled !"
            return
        fi
        let "++count"
    fi
    log "Upgrade failed ${count} times !"
    if [ "${count}" -ge "${count_max}" ];then
        log "Exit recovery !"
        unset_recovery
        sync
        
        echo 0 > /sys/class/backlight/backlight/brightness
        reboot
    fi
    echo "${count}" > "${count_filepath}"
}

report() {
    log "Start report($*)..."
    case "$1" in
        'ok')
            emu_report 'ok'
            ui_show 'ok'
            if is_recovery;then
                unset_recovery
            fi
             
            # 升级成功后删除升级包
            if [ -f "${ZIP_FILE}" ];then
                rm "${ZIP_FILE}"
            fi
        
            sync
            
            echo 0 > /sys/class/backlight/backlight/brightness
            reboot
            exit 0
            ;;srvs
        'error')
            emu_report 'error'
            ui_show 'error'
            if ! is_recovery;then
                # 未在 recovery 模式, 升级失败后删除升级包
                if [ -f "${ZIP_FILE}" ];then
                    rm "${ZIP_FILE}"
                fi
                # 未在 recovery 模式, 升级失败后启动 monit (刚好 vcpe 需重启才能上报结果)
                source '/etc/profile.d/RkEnv.sh'
                monit
            else
                check_count
            fi
            sync
            exit 1
            ;;
        *)
            log "report() unknown args: [$*]"
            ;;
    esac
}

sys_prepare() {
    srvs="vodsl gui eaServer goahead addrbook_sync syslogd klogd udevd adbd"

    log "stop: $srvs"
    aim stop
    killall -9 'monit'
    killall -9 $srvs
    echo 3 > /proc/sys/vm/drop_caches
}

check_space_is_enough() {
    log "Start check_space_is_enough($*)..."
    local img_name="$1"
    local mount_node="$2"

    local img_size="$(unzip -l "${ZIP_FILE}" | grep -E "[[:space:]]${img_name}\$" | awk '{print $1}')"
    local remain_size="$(df -Pk | grep -E "[[:space:]]${mount_node}$" | awk '{printf $4}')"

    if [ -z "${img_size}" ] || [ -z "${remain_size}" ];then
        log "Get size error: img_size[${img_size}] remain_size[${remain_size}] !"
        return 1
    fi

    # 预留空间, 计算空间是否充足时, 分区大小会先减去这部分大小
    local reserved_kb
    local ring_dir="/userdata/ring"
    local ring_size
    case "${mount_node}" in
        '/userdata')
            # 预留 32k 用来保存日志的空间
            reserved_kb='32'
            ring_size=$(du -s $ring_dir | awk '{print $1}')
            ;;
        '/tmp')
            reserved_kb='5120'
            ring_size='0'
            ;;
        *)
            reserved_kb='0'
            ring_size='0'
            ;;
    esac
    log "Space [${mount_node}]: ${remain_size}kb    Reserved: ${reserved_kb}kb    Ring: ${ring_size}kb"
    # 剩余空间系统自用一部分, 比例约为 0.4%, 这里预先减去 1%
    let 'remain_size = remain_size * 99 / 100'
    # 减去预留空间, 同时转换单位 kb 为 b
    let 'remain_size = (remain_size - reserved_kb) * 1024'
    if [ "${img_size}" -gt "${remain_size}" ];then
        # 剩余空间加上铃声所占空间判断是否够升级
        let 'remain_size = remain_size + ring_size * 1024'
        if [ "${img_size}" -le "${remain_size}" ];then
            # 清空导入铃声
            rm -rf "${ring_dir}"/*
            log "Space [${mount_node}] is't enough: rm -rf ${ring_dir}/* !"
        else
            # 没清空铃声，将铃声的空间减回去
            let 'remain_size = remain_size - ring_size * 1024'
            log "Space [${mount_node}] is't enough: img_size[${img_size}] > remain_size[${remain_size}] !"
            return 1
        fi
    fi

    log "Space [${mount_node}]: ${remain_size}kb is enough for '${img_name}' ${img_size}kb"
    return 0
}

mount_oem() {
    local oem_device='/dev/block/by-name/oem'
    local empty=0

    [ $# -gt 0 ] && empty=$1

    # 检验分区是否已挂载
    if ! df | grep -qE "[[:space:]]${oem_path}$";then
        # 格式化 oem 
        if [ "$empty" -eq 1 ]; then
            if ! mkfs.ext2 -qFm 0 "${oem_device}";then
                log "mkfs.ext2 -qFm 0 '${oem_device}' failed !"
                return 1
            fi
        fi

        # 挂载 oem 分区
        mkdir -p "${oem_path}"
        if ! mount "${oem_device}" "${oem_path}";then
            log "mount '${oem_device}' '${oem_path}' failed !"
            return 1
        fi
        return 0
    fi

    # 已挂载，直接清空即可
    if [ "$empty" -eq 1 ]; then
        # 清空 oem 分区
        rm -rf "${oem_path}"/*
    fi
    return 0
}

check_md5sum_valid() {
    local file="$1"
    local md5_cfg="$2"
    local md5_cur

    [ ! -f "$file" ] && return 1

    if [ $# -ge 3 ]; then
        local file_name="$2"
        local md5_file="$3"

        [ ! -f "$md5_file"  ] && return 1
        md5_cfg=$(awk '/'"$file_name"'/{print $1}' "$md5_file")
    fi
    md5_cur=$(md5sum "$file" | awk '{print $1}')

    if [ "$md5_cfg" != "$md5_cur" ]; then
        log "$file is invalid. (md5sum: config [$md5_cfg], current [$md5_cur])"
        return 1
    fi

    return 0
}

unzip_big_img() {
    log "Start unzip_big_img($*)..."
    local big_img_name="$1"
    local big_img_path="$2"

    # 清空 oem 分区后不能退出 recovery 模式
    echo "${exit_disabled_str}" > "${count_filepath}"

    # 挂载oem分区
    if ! mount_oem 1; then
        return 1
    fi

    # 检验 oem 分区空间是否足够
    if ! check_space_is_enough "${big_img_name}" "${oem_path}";then
        return 1
    fi
    # 解压 img 到 oem 分区
    if ! unzip -o "${ZIP_FILE}" "${big_img_name}" -d "${oem_path}";then
        log "Unzip '${ZIP_FILE}' to '${oem_path}/${big_img_name}' failed !"
        return 1
    fi

    if [ ! -f "${UPGRADEFILE}/md5sum.txt" ]; then
        unzip ${ZIP_FILE} md5sum.txt -d ${UPGRADEFILE}
    fi

    # 根据md5sum.txt，检查img是否完整
    if ! check_md5sum_valid "${oem_path}/${big_img_name}" "${big_img_name}" "${UPGRADEFILE}/md5sum.txt"; then
        return 1
    fi

    if [ "${big_img_name}" != 'zip_oem.img' ];then
        # 创建软连接到解压在 oem 分区的 img
        if ! ln -sf "${oem_path}/${big_img_name}" "${big_img_path}";then
            log "ln -sf '${oem_path}/${big_img_name}' '${big_img_path}' failed !"
            return 1
        fi
    else
        # 升级 oem 分区时不能挂载 oem 分区
        # 删除 zip 升级包文件
        if ! rm "${ZIP_FILE}";then
            log "rm '${ZIP_FILE}' failed !"
            return 1
        fi

        # 异常恢复：升级过程中掉电进入商用系统，但oem实际上未烧写成功，zip_oem.img还在，设备无法正常启动，则重新烧写oem
        mkdir -p "${oem_path}/etc/init.d"
        ln -s "${UPGRADEFILE}/update.sh" "${oem_path}/etc/init.d/rcS"
        # 创建一个假升级包，确保升级过程中掉电进入Recovery系统，能进入update.sh oem升级流程
        touch $fake_zip

        # 将 oem 分区的 img 移动回 userdata 分区
        if ! mv "${oem_path}/${big_img_name}" "${big_img_path}";then
            log "mv '${oem_path}/${big_img_name}' '${big_img_path}' failed !"
            return 1
        fi
        # 卸载 oem 分区
        if ! umount "${oem_path}";then
            log "umount '${oem_path}' failed !"
            return 1
        fi
        sync
    fi

    log "Unzip big img[${big_img_path}] successed !"
    return 0
}

update_engine()
{
    local img_path="$1"
    local img_partition="$2"

    /usr/bin/updateEngine '--update' "--image_url=${img_path}" "--partition=${img_partition}"
    if [ "$?" -ne 0 ];then
        log "updateEngine failed !"
        rm "${img_path}"
        return 1
    fi

    if ! rm "${img_path}";then
        log "rm '${img_path}' failed !"
        return 1
    fi

    log "Upgrade '${img_path}' successed !"
    return 0
}

upgrade_img() {
    log "Start upgrade_img($*)..."
    local img_dirname='/tmp'
    local img_name="zip_$1.img"
    local img_path="${img_dirname}/${img_name}"
    local img_partition="0x3B00"

    log '======================================================'
    log "Start upgrade '${img_path}'..."

    if [ "$1" = "recovery" ];then
        # 检查是否需要升级 recovery
        if "${upgrade_check}" recovery;then
            log "Don't need to upgrade recovery !"
            return 0
        fi
        img_partition="0x0400"
    fi

    local enough='true'

    if ! check_space_is_enough "${img_name}" '/tmp';then
        if ! check_space_is_enough "${img_name}" '/userdata';then
            enough='false'
        fi
        img_dirname='/userdata/upgradefile'
        img_path="${img_dirname}/${img_name}"
        log "Change img_path to [${img_path}] !"
    fi

    if ${enough};then
        if ! unzip -o "${ZIP_FILE}" "${img_name}" -d "${img_dirname}";then
            log "Unzip '${ZIP_FILE}' to '${img_path}]' failed !"
            rm -f "${img_path}"
            return 1
        fi
    else
        # 注意: 此处会删除升级包
        if ! unzip_big_img "${img_name}" "${img_path}";then
            log "unzip_big_img failed !"
            return 1
        fi
    fi

    if ! update_engine "${img_path}" "${img_partition}"; then
        return 1
    fi
    return 0
}

upgrade_recovery() {
    # 检查软件硬件版本
    if ! "${upgrade_check}" upgrade;then
        log "Zip version error."
        report 'error'
    fi

    ui_show file

    # 尝试升级 recovery
    if ! upgrade_img recovery;then
        log "upgrade_img recovery error."
        report 'error'
    fi

    # 设置 recovery 标志, 成功则重启
    if set_recovery;then
        cp -f /var/kmod/panel-tm028.ko /userdata
        cp -f /var/kmod/rockchip_rgb.ko /userdata

        # 升级开始时删除统计失败次数的文件
        rm -f "${count_filepath}"
        sync;reboot
        exit 0
    fi

    report 'error'
}

upgrade_oem() {
    local md5file="$UPGRADEFILE/md5sum.txt"
    local oem_md5=""
    local oem_img="$UPGRADEFILE/zip_oem.img"
    local zip_oem_img="$oem_path/zip_oem.img"
    local sys_mode="Recovery"

    if ! is_recovery; then
        sys_mode="Business"
    fi

    # 输出转储到日志文件
    exec 1>>"${log_file}" 2>&1
    log '======================================================'
    log "Enter $sys_mode Mode: oem upgrade start..."

    #LCD显示加载文件图片
    ui_show file       

    if [ ! -f "$md5file" ]; then
        log "$md5file not found."
        report 'error'
    fi

    oem_md5=$(awk '/zip_oem.img/{print $1}' $md5file)
    if [ -z "$oem_md5" ]; then
        log "$md5file doesn't have zip_oem.img information."
        #LCD显示upgrade_error.png
        report 'error'
    fi

    # 挂载oem分区
    if ! mount_oem; then
        report 'error'
    fi

    ui_show 'step' "15000"

    # 根据md5sum.txt，检查/userdata/upgradefile/zip_oem.img是否完整
    if ! check_md5sum_valid "$oem_img" "$oem_md5"; then

        # 根据md5sum.txt，检查/userdata/oem/zip_oem.img是否完整
        if ! check_md5sum_valid "$zip_oem_img" "$oem_md5"; then
            report 'error'
        fi
        cp "$zip_oem_img" "$oem_img"
        log "upgrade oem from $zip_oem_img"
    fi

    if ! update_engine "$oem_img" "0x3B00"; then
        report 'error'
    fi

    rm -rf "$UPGRADEFILE/*"
    rm -rf "$fake_zip"

    # 卸载oem分区，避免重启时，调用/oem/etc/init.d/rcS
    umount "$oem_path"
    umount "/oem"

    #LCD显示upgrade_ok.png图片
    report 'ok'
}

main() {
    # 如果升级包中不存在升级检查脚本, 则将 /oem/bin 中的移动过去
    if ! [ -f "${upgrade_check}" ];then
        cp "/oem/bin/${upgrade_check_script}" "${upgrade_check}"
    fi
    if [ -f "${upgrade_check}" ];then
        chmod +x "${upgrade_check}"
    else
        log "Don't find ${upgrade_check_script} !"
        report 'error'
    fi

    # 不是 recovery 模式时检查版本, 并重启进入 recovery 模式
    if ! is_recovery;then
        upgrade_recovery
    fi

    # 输出转储到日志文件
    exec 1>>"${log_file}" 2>&1
    log '======================================================'
    log 'Enter Recovery Mode: zip upgrade start...'

    # 加载内核模块
    [ -f '/userdata/panel-tm028.ko' ] && insmod '/userdata/panel-tm028.ko'
    [ -f '/userdata/rockchip_rgb.ko' ] && insmod '/userdata/rockchip_rgb.ko'

    #LCD显示1/10-10/10
    get_ui_show_step_value
    
    ui_show 'step' "${ui_show_step_value}" 
    
    # 逐个升级 img (oem 分区放最后, 原因见 unzip_big_img 函数)
    imgs="boot oem"
    for i in ${imgs};do
        if ! upgrade_img "$i";then
            report 'error'
        fi
    done
    report 'ok'
}

if [ -f "$UPGRADEFILE/upgrade_ui" ]; then
    echo "----upgrade.sh: $UPGRADEFILE/upgrade_ui exist"

    mv -f $UPGRADEFILE/upgrade_ui       /userdata/upgrade_ui
    echo "----upgrade.sh: mv -f $UPGRADEFILE/upgrade_ui  TO /userdata/upgrade_ui"
    if [ -f "$UPGRADEFILE/boot_animation" ]; then
        mv -f $UPGRADEFILE/boot_animation   /userdata/ui
        echo "----upgrade.sh: mv -f $UPGRADEFILE/boot_animation  TO /userdata/ui"    
    fi

    if [ -f "$UPGRADEFILE/img_print" ]; then
        mv -f $UPGRADEFILE/img_print       /userdata/img_print
        echo "----upgrade.sh: mv -f $UPGRADEFILE/img_print  TO /userdata/img_print"    
    fi

    if [ -d "$UPGRADEFILE/img" ]; then
        [ -d "/userdata/img" ] && rm -rf /userdata/img
        mv -f $UPGRADEFILE/img          /userdata/
    fi
elif [ -f "/oem/bin/upgrade_ui" ] && [ ! -f "/userdata/upgrade_ui" ]; then
    echo "----upgrade.sh: $UPGRADEFILE/upgrade_ui not found"
    cp -rf /oem/bin/upgrade_ui          /userdata/upgrade_ui
    echo "----upgrade.sh: cp -rf /oem/bin/upgrade_ui  TO /userdata/upgrade_ui"
    cp -rf /oem/bin/boot_animation      /userdata/ui
    echo "----upgrade.sh: cp -rf /oem/bin/upgrade_ui  TO /userdata/upgrade_ui"
    rm -rf /userdata/img
    cp -rf /oem/img                     /userdata/

    if [ -f "/oem/bin/img_print" ]; then
        mv -f /oem/bin/img_print       /userdata/img_print
        echo "----upgrade.sh: cp -rf /oem/bin/img_print  TO /userdata/img_print"  
    fi

fi

if [ ! -f $ZIP_FILE ]; then
    if [ ! -f "/userdata/BulkUpgrade.zip" ]; then
        upgrade_oem
        exit 0
    fi
    ZIP_FILE="/userdata/BulkUpgrade.zip"
fi
sys_prepare

main "$@"
