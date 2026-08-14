# نظام القفازات والأكمام (Gloves & Sleeves Loadout System)

## نظرة عامة
تم إضافة نظام يسمح للاعبين بتخصيص القفازات والأكمام لفريقي الإرهابيين (T) ومكافحة الإرهابيين (CT) في Counter-Strike: Source.

## المتغيرات الأربعة (ConVars)

### 1. `loadout_gloves_t`
- **الوصف**: مسار نموذج القفازات لفريق الإرهابيين
- **القيمة الافتراضية**: فارغ
- **مثال**: `models/arms/custom/gloves/v_glove_bloodhound.mdl`

### 2. `loadout_gloves_ct`
- **الوصف**: مسار نموذج القفازات لفريق مكافحة الإرهابيين
- **القيمة الافتراضية**: فارغ
- **مثال**: `models/arms/custom/gloves/v_glove_sporty.mdl`

### 3. `loadout_sleeves_t`
- **الوصف**: مسار نموذج الأكمام لفريق الإرهابيين
- **القيمة الافتراضية**: فارغ
- **مثال**: `models/arms/custom/sleeves/anarchist/v_sleeve_anarchist.mdl`

### 4. `loadout_sleeves_ct`
- **الوصف**: مسار نموذج الأكمام لفريق مكافحة الإرهابيين
- **القيمة الافتراضية**: فارغ
- **مثال**: `models/arms/custom/sleeves/fbi/v_sleeve_fbi.mdl`

## كيفية الاستخدام

### 1. وضع الملفات في المجلدات الصحيحة
```
cstrike/
├── models/
│   └── arms/
│       ├── custom/
│       │   ├── gloves/
│       │   │   ├── v_glove_bloodhound.mdl
│       │   │   └── ...
│       │   └── sleeves/
│       │       ├── anarchist/
│       │       │   └── v_sleeve_anarchist.mdl
│       │       └── fbi/
│       │           └── v_sleeve_fbi.mdl
└── materials/
    └── arms/
        ├── glove_bloodhound/
        │   ├── glove_bloodhound.vmt
        │   └── glove_bloodhound.vtf
        └── anarchist/
            ├── sleeve_anarchist.vmt
            └── sleeve_anarchist.vtf
```

### 2. تعيين القفازات والأكمام عبر الكونسول

#### لفريق الإرهابيين (T):
```bash
loadout_gloves_t "models/arms/custom/gloves/v_glove_bloodhound.mdl"
loadout_sleeves_t "models/arms/custom/sleeves/anarchist/v_sleeve_anarchist.mdl"
```

#### لفريق مكافحة الإرهابيين (CT):
```bash
loadout_gloves_ct "models/arms/custom/gloves/v_glove_sporty.mdl"
loadout_sleeves_ct "models/arms/custom/sleeves/fbi/v_sleeve_fbi.mdl"
```

### 3. الحفظ التلقائي
بفضل استخدام `FCVAR_ARCHIVE`، سيتم حفظ هذه الإعدادات تلقائياً في ملف `config.cfg` ولن تحتاج إلى إعادة تعيينها في كل مرة.

## الملفات المعدلة

### 1. `game/client/cstrike/c_cs_player.cpp`
- إضافة المتغيرات الأربعة (ConVars)
- إضافة دالة `UpdateLoadoutGlovesAndSleeves()`
- تحديث دالة `PostDataUpdate()` لاستدعاء النظام عند تغيير الفريق

### 2. `game/client/cstrike/c_cs_player.h`
- إعلان دالة `UpdateLoadoutGlovesAndSleeves()`
- إضافة متغير `m_iLastTeamNumber` لتتبع تغييرات الفريق

## آلية العمل

1. **عند انضمام اللاعب للعبة**: يتم تهيئة `m_iLastTeamNumber` إلى `TEAM_UNASSIGNED`
2. **عند تغيير الفريق**: تستدعي `PostDataUpdate()` دالة `UpdateLoadoutGlovesAndSleeves()`
3. **تحديث القفازات والأكمام**: تقرأ الدالة المتغيرات المناسبة للفريق الحالي وتطبق النماذج

## أمثلة على المسارات المتاحة

بناءً على حزمة الموديلات التي لديك:

### القفازات (Gloves):
- `models/arms/custom/gloves/v_glove_bloodhound.mdl`
- `models/arms/custom/gloves/v_glove_sporty.mdl`
- `models/arms/custom/gloves/v_glove_motorcycle.mdl`
- `models/arms/custom/gloves/v_glove_specialist.mdl`
- `models/arms/custom/gloves/v_glove_hardknuckle.mdl`
- `models/arms/custom/gloves/v_glove_hydra.mdl`
- `models/arms/custom/gloves/v_glove_brokenfang.mdl`

### الأكمام (Sleeves):
- `models/arms/custom/sleeves/anarchist/v_sleeve_anarchist.mdl`
- `models/arms/custom/sleeves/balkan/v_sleeve_balkan.mdl`
- `models/arms/custom/sleeves/fbi/v_sleeve_fbi.mdl`
- `models/arms/custom/sleeves/gign/v_sleeve_gign.mdl`
- `models/arms/custom/sleeves/sas/v_sleeve_sas.mdl`
- `models/arms/custom/sleeves/swat/v_sleeve_swat.mdl`

## ملاحظات هامة

1. **التوافق**: تأكد من أن جميع ملفات `.mdl` و `.vtx` و `.vvd` و `.materials` موجودة
2. **الأداء**: استخدام نماذج خفيفة لتحسين الأداء
3. **السيرفرات**: قد تحتاج بعض السيرفرات إلى تعديل إضافي لدعم النظام بالكامل
4. **الرؤية**: القفازات والأكمام تظهر فقط للاعب نفسه (على الـ ViewModel)

## استكشاف الأخطاء

إذا لم تظهر القفازات أو الأكمام:
1. تحقق من صحة المسار في الكونسول
2. تأكد من وجود جميع ملفات الموديل
3. استخدم الأمر `reload` لإعادة تحميل المواد
4. تحقق من الكونسول لأي رسائل خطأ

## التطوير المستقبلي

يمكن تحسين النظام بإضافة:
- دعم لاختيار القفازات من قائمة في اللعبة
- معاينة القفازات قبل الشراء
- دعم للقفازات النادرة والمميزة
- تأثيرات خاصة للقفازات
