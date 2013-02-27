#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmath.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::calculate_stuff();
    ui->warnings->setText("Armsman has been preloaded with an example character. Change the weapon, feats, and stats then select an attack below and click 'Calculate Damage.' Please send any comments or bug reports to RussellChamp@gmail.com");

    //Start several elements off invisible
    ui->attack_great_cleave_targets_label->setVisible(false);
    ui->attack_great_cleave_targets->setVisible(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

int get_hitchance(int hit_bonus, int target_ac)
{
    //(20 + 1 + hit_bonus - target_ac) * 100 / 20
    //+1 since ties HIT
    //min 5% (crit hit), max 95% (crit miss)
    int hit_percent = qBound(5, (21 + hit_bonus - target_ac)*5, 95);

    return hit_percent;
}

int get_critchance(int hit_bonus, int crit_confirm_bonus,int crit_min, int target_ac, bool total_chance = false)
{
    //when total_chance is set, it will return the average crit chance for *any* attack and not just successful attacks
    //successful hit rolls = the number of sides on a d20 that would result in a successful hit
    //successful hit rolls = 21 + hit_bonus - target_ac (max 19 (crit miss), min 1 (crit hit))
    int successful_hits = qBound(1, 21 + hit_bonus - target_ac, 19);
    //out of those, (21 - crit_min) will be crits (max successful hits, min should be 1)
    int crits = qBound(1, 21 - crit_min, successful_hits);
    //so the chance will be
    int crit_chance = crits * 100 / successful_hits; //of all successful hits

    //eg. hit chance: (21 + 10 hit bonus - 21 ac) = 10 successful hits
    //eg. crit chance 19-20/x2 : 21 ac - 19 crit min = 2 crit hits
    //2 crit hits out of 10 successful hits = 20% of all successful hits are crits

    // crit confirm: get_hitchance(hit_bonus+crit_confirm_bonus, target_ac) to confirm crit
    //eg. (21 + 10 hit bonus + 4 crit confirm bonus - 21 ac) * 100 / 20 = 70% crit confirm
    //crit_chance * crit_confirm
    //20% * 70% = 14% of all successful hits are confirmed crits

    if(total_chance)
        return crit_chance * get_hitchance(hit_bonus + crit_confirm_bonus, target_ac) * get_hitchance(hit_bonus, target_ac) / 10000;
    else
        return crit_chance * get_hitchance(hit_bonus + crit_confirm_bonus, target_ac) / 100;
}

float MainWindow::get_damage(bool crit = false)
{
    float damage = 0;

    //Damage multiplied by critical hits
    if(ui->feat_arcanestrike->checkState())
        damage += ui->feat_arcanestrike_bonus->value();
    if(ui->feat_weaponspecialization->checkState())
        damage += 2;
    if(ui->feat_greaterweaponspecialization->checkState())
        damage += 2;
    if(ui->feat_weapontraining->checkState())
        damage += ui->feat_weapontraining_bonus->value();
    if(ui->weapon_melee->isChecked())
    {
        if(ui->weapon_melee_twohanded->checkState())
            damage += qFloor(1.5 * ui->stat_str_mod->value());
        else
            damage += ui->stat_str_mod->value();
        if(ui->attack_type->currentText() == "Power Attack")
        {
            if(ui->weapon_melee_twohanded->checkState())
                damage += qFloor((4 + ui->attack_1_bonus->value())/4)*3;
            else
                damage += qFloor((4 + ui->attack_1_bonus->value())/4)*2;
        }
    }
    else if(ui->weapon_ranged->isChecked() && ui->weapon_ranged_composite->isChecked())
        damage += qMin(ui->weapon_ranged_composite_bonus->value(), ui->stat_str_mod->value());

    damage += ui->weapon_bonus->value();
    damage += ui->misc_bonus_damage_crit->value();

    //average damage for the weapon's die roll will be the number of dies rolled
    //times the average damage per die, which can be simplified to the number of
    //die sides + 1 then divided by 2
    damage += ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1)/2.0;

    if(crit)
    {
        //multiply for crit damage
        damage *= ui->crit_multiplier->value();

        //special damage only during crits
        if(ui->weapon_flamingburst->checkState())
            damage += qBound(1, ui->crit_multiplier->value() - 1, 3) * 5.5; //(crit multiplier - 1)d10
        if(ui->weapon_thundering->checkState())
            damage += qBound(1, ui->crit_multiplier->value() - 1, 3) * 4.5; //(crit multiplier - 1)d8
    }

    //Damage that is NOT multiplied during a crit hit.
    if(ui->weapon_flaming->checkState())
        damage += 3.5; //1d6
    if(ui->weapon_holy->checkState())
        damage += 7; //2d6
    if(ui->weapon_vicious->checkState())
        damage += 7; //2d6
    if(ui->weapon_anarchic->checkState())
        damage += 7; //2d6
    if(ui->weapon_wounding->checkState())
        damage += 1; //1 point bleed
    if(ui->feat_sneakattack->isChecked())
        damage += ui->feat_sneak_attack_bonus->value() * 3.5; //sneak bonus d6
    damage += ui->misc_bonus_damage_noncrit->value();

    return damage;
}

void MainWindow::on_pushButton_calculate_clicked()
{
    calculate_stuff();
}

void MainWindow::calculate_stuff()
{
    get_warnings();

    QString results = "";
    QString attack_type = ui->attack_type->currentText();
    QString weapon = "";
    QString weapon_damage = "";
    QString attacks = ""; //bonuses on each attack
    QString hit_chance = ""; //the chance each attack will hit
    QString crit_chance = ""; //the chance that a successful hit will be a crit
    QString crit_chance_all = ""; //the chance that *any* attack will be a crit
    int enemy_ac = 0;
    int attacks_num = 0; //not really used. will probably phase out
    int hit_bonus = 0; //the total to-hit bonus applied to all attacks

    if(ui->weapon_masterwork->checkState())
        weapon.append("Masterwork ");
    if(ui->weapon_bonus->value() > 0)
        weapon.append("+" + QString::number(ui->weapon_bonus->value()) + " ");
    weapon.append(ui->weapon_name->text());

    weapon_damage.append(" (");
    weapon_damage.append(QString::number(ui->weapon_die_count->value()) + "d" + ui->weapon_die_value->currentText() + " ");

    int weapon_damage_bonus = 0;
    if(ui->weapon_melee->isChecked())
    {
        if(ui->weapon_melee_twohanded->checkState())
            weapon_damage_bonus += qFloor(1.5 * ui->stat_str_mod->value());
        else
            weapon_damage_bonus += ui->stat_str_mod->value();
        if(ui->attack_type->currentText() == "Power Attack")
        {
            if(ui->weapon_melee_twohanded->checkState())
                weapon_damage_bonus += qFloor((4 + ui->attack_1_bonus->value())/4)*3;
            else
                weapon_damage_bonus += qFloor((4 + ui->attack_1_bonus->value())/4)*2;
            hit_bonus -= qFloor((4 + ui->attack_1_bonus->value())/4);
        }
    }
    else if(ui->weapon_ranged->isChecked() && ui->weapon_ranged_composite->isChecked())
    {
        if(ui->stat_str_mod->value() < ui->weapon_ranged_composite_bonus->value())
            hit_bonus -= 2;
        weapon_damage_bonus += qMin(ui->weapon_ranged_composite_bonus->value(), ui->stat_str_mod->value());
    }

    weapon_damage_bonus += ui->weapon_bonus->value();
    if(ui->feat_weaponspecialization->checkState())
        weapon_damage_bonus += 2;
    if(ui->feat_greaterweaponspecialization->checkState())
        weapon_damage_bonus += 2;
    if(ui->feat_weapontraining->checkState())
    {
        weapon_damage_bonus += ui->feat_weapontraining_bonus->value();
        hit_bonus += ui->feat_weapontraining_bonus->value();
    }
    if(ui->feat_arcanestrike->checkState())
        weapon_damage_bonus += ui->feat_arcanestrike_bonus->value();

    weapon_damage_bonus += ui->misc_bonus_damage_crit->value() + ui->misc_bonus_damage_noncrit->value();
    weapon_damage.append("+ " + QString::number(weapon_damage_bonus));

    if(ui->weapon_flaming->checkState())
        weapon_damage.append(" + 1d6 (Flaming)");
    if(ui->weapon_flamingburst->checkState())
        weapon_damage.append(" + " + QString::number(qBound(1, ui->crit_multiplier->value() - 1, 3)) + "d10 on crit (Burst)");
    if(ui->weapon_thundering->checkState())
        weapon_damage.append(" + " + QString::number(qBound(1, ui->crit_multiplier->value() - 1, 3)) + "d8 on crit (Thundering)");
    if(ui->weapon_holy->checkState())
        weapon_damage.append(" + 2d6 (Un/Holy)");
    if(ui->weapon_anarchic->checkState())
        weapon_damage.append(" + 2d6 (Anarchic/Axiomatic)");
    if(ui->weapon_vicious->checkState())
        weapon_damage.append(" + 2d6 (Vicious)");
    if(ui->feat_sneakattack->isChecked())
        weapon_damage.append(" + " + QString::number(ui->feat_sneak_attack_bonus->value()) + "d6 (Sneak Attack)");
    if(ui->weapon_wounding->checkState())
        weapon_damage.append(" +1 bleed (Wounding, stacks)");
    if(ui->feat_bleedingattack->isChecked())
        weapon_damage.append(" + " + QString::number(ui->feat_sneak_attack_bonus->value()) + " bleed (Bleeding Attack)");

    weapon_damage.append(")");

    enemy_ac = ui->enemy_ac->value();

    hit_bonus += ui->misc_bonus_attack->value() + ui->weapon_bonus->value();
    if(ui->weapon_masterwork->checkState() && ui->weapon_bonus->value() == 0)
        hit_bonus++;
    if(ui->feat_weaponfocus->checkState())
        hit_bonus++;
    if(ui->feat_greaterweaponfocus->checkState())
        hit_bonus++;
    if(ui->weapon_melee->isChecked())
    {
        if(ui->feat_weaponfinesse->checkState() && ui->stat_dex_mod->value() > ui->stat_str_mod->value())
            hit_bonus += ui->stat_dex_mod->value();
        else
            hit_bonus += ui->stat_str_mod->value();
    }
    else if(ui->weapon_ranged->isChecked())
        hit_bonus += ui->stat_dex_mod->value();

    int crit_min_bound = ui->crit_range_min->value();
    if(ui->feat_improvedcritical->checkState() || ui->weapon_keen->checkState())
        crit_min_bound = qMax(1, crit_min_bound * 2 - 21); //21 - (21 - crit_min_bound) * 2
    int crit_confirm_bonus = 0;
    if(ui->feat_criticalfocus->checkState())
        crit_confirm_bonus += 4;

    if(ui->buff_haste->isChecked())
        hit_bonus += 1;

    if(attack_type == "Rapid Shot")
        hit_bonus -= 2;

    int guidance_bonus = 0;
    if(ui->buff_guidance->isChecked()) //only works on first attack
        guidance_bonus = 1;

    attacks.append("+" + QString::number(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus));
    hit_chance.append(QString::number(get_hitchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, enemy_ac)) + "%");
    crit_chance.append(QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
    crit_chance_all.append(QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");


    attacks_num++;

    if((attack_type == "Full Attack" || attack_type == "Power Attack" || attack_type == "Vital Strike" || attack_type == "Improved Vital Strike"
        || attack_type == "Greater Vital Strike" || attack_type == "Rapid Shot" || attack_type == "Manyshot") && ui->attack_2_bonus->value() > 0)
    {
        if((attack_type == "Full Attack" || attack_type == "Rapid Shot") && (ui->weapon_speed->isChecked() || ui->buff_haste->isChecked()))
        { //perform an extra attack during "Full Attack" or "Rapid Shot" when you either have a weapon with speed or are haste'd
            attacks.append("/+" + QString::number(ui->attack_1_bonus->value() + hit_bonus));
            hit_chance.append("/" + QString::number(get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)) + "%");
            crit_chance.append("/" + QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
            crit_chance_all.append("/" + QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");
            attacks_num++;
        }
        if(attack_type == "Rapid Shot")
        { //perform an extra attack when using "Rapid Shot"
            attacks.append("/+" + QString::number(ui->attack_1_bonus->value() + hit_bonus));
            hit_chance.append("/" + QString::number(get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)) + "%");
            crit_chance.append("/" + QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
            crit_chance_all.append("/" + QString::number(get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");
            attacks_num++;
        }

        attacks.append("/+" + QString::number(ui->attack_2_bonus->value() + hit_bonus));
        attacks_num++;
        hit_chance.append("/" + QString::number(get_hitchance(ui->attack_2_bonus->value() + hit_bonus, enemy_ac)) + "%");
        crit_chance.append("/" + QString::number(get_critchance(ui->attack_2_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
        crit_chance_all.append("/" + QString::number(get_critchance(ui->attack_2_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");

        if(ui->attack_3_bonus->value() > 0)
        {
            attacks.append("/+" + QString::number(ui->attack_3_bonus->value() + hit_bonus));
            attacks_num++;
            hit_chance.append("/" + QString::number(get_hitchance(ui->attack_3_bonus->value() + hit_bonus, enemy_ac)) + "%");
            crit_chance.append("/" + QString::number(get_critchance(ui->attack_3_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
            crit_chance_all.append("/" + QString::number(get_critchance(ui->attack_3_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");

            if(ui->attack_4_bonus->value() > 0)
            {
                attacks.append("/+" + QString::number(ui->attack_4_bonus->value() + hit_bonus));
                attacks_num++;
                hit_chance.append("/" + QString::number(get_hitchance(ui->attack_4_bonus->value() + hit_bonus, enemy_ac)) + "%");
                crit_chance.append("/" + QString::number(get_critchance(ui->attack_4_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)) + "%");
                crit_chance_all.append("/" + QString::number(get_critchance(ui->attack_4_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac, true)) + "%");

            }
        }
    }

    results.append("Weapon: " + weapon + weapon_damage + "\nAttack(s): " + attacks +
                   "\nHit Chance: " + hit_chance + " of attacks" +
                   "\nCrit Chance " + crit_chance + " of successful hits" +
                   "\n                 " + crit_chance_all + " of all attacks" +
                   "\n----- " + ui->attack_type->currentText() + " -----" +
                   "\n");

    float damage_noncrit = get_damage();
    float damage_crit = get_damage(true);
    float manyshot_bonus = 0;
    float damage_expected = 0;
    float damage_total = 0;

    if(attack_type == "Vital Strike")
    {
        //roll the damage dice again for the vital strike
        //TODO: figure out which (if ANY) bonuses should be multiplied for vital strike (eg the weapon damage bonus)
        //for now, I am assuming that NO bonuses are multiplied
        damage_noncrit += ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1)/2.0;
        damage_crit += ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1)/2.0;
    }
    else if(attack_type == "Improved Vital Strike")
    {
        //roll the damage dice twice more for improved vital strike
        damage_noncrit += ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1);
        damage_crit += ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1);
    }
    else if(attack_type == "Greater Vital Strike")
    {
        //roll the damage dice thrice more for greater vital strike
        damage_noncrit += 1.5 * ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1);
        damage_crit += 1.5 * ui->weapon_die_count->value() * (ui->weapon_die_value->currentText().toInt() + 1);
    }
    else if(attack_type == "Deadly Stroke")
    {
        if(!ui->feat_greaterweaponfocus->checkState())
            ui->warnings->setText("Warning: Greater Weapon Focus is required to perform Deadly Stroke");
        //deal double the normal damage. additional damage is not multiplied on a crit
        damage_crit += damage_noncrit;
        damage_noncrit *= 2;
    }


    if(attack_type == "Manyshot")
    {
        if(ui->stat_dex_mod->value() < 3)
            ui->warnings->setText("Warning: Manyshot requires a dex of at least 17");
        manyshot_bonus = damage_noncrit;
        if(ui->feat_sneakattack->isChecked())
            manyshot_bonus -= 3.5 * ui->feat_sneak_attack_bonus->value(); //sneak is only counted once
        results.append("Average damage (non-crit): " + QString::number(damage_noncrit + manyshot_bonus, 'g', 4) + " (first shot) / " + QString::number(damage_noncrit, 'g', 4) + " (other shots)" +
                       "\nAverage damage (crit): " + QString::number(damage_crit + manyshot_bonus, 'g', 4) + " (first shot) / " + QString::number(damage_crit, 'g', 4) + " (other shots)" +
                       "\n");
        damage_expected = (damage_noncrit + manyshot_bonus + (damage_crit - damage_noncrit) * get_critchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, enemy_ac)/100.0;
    }
    else
    {
        results.append("Average damage (non-crit): " + QString::number(damage_noncrit, 'g', 4) +
                      "\nAverage damage (crit): " + QString::number(damage_crit, 'g', 4) +
                      "\n");
        //Expected damage per turn = (average non-crit damage + bonus crit damage * chance to crit)* chance to hit
        //                         = (non-crit + (crit - non-crit) * crit chance) * hit chance
        damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_1_bonus->value() + hit_bonus + guidance_bonus, enemy_ac)/100.0;
    }
    damage_total = damage_expected;
    results.append("Expected damage: " + QString::number(damage_expected, 'g', 4));


    if(attack_type == "Full Attack" || attack_type == "Power Attack" || attack_type == "Vital Strike" || attack_type == "Improved Vital Strike"
            || attack_type == "Greater Vital Strike" || attack_type == "Rapid Shot" || attack_type == "Manyshot")
    {
        if((attack_type == "Full Attack" || attack_type == "Rapid Shot") && (ui->weapon_speed->isChecked() || ui->buff_haste->isChecked()))
        {
            damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)/100.0;
            results.append("/" + QString::number(damage_expected, 'g', 4));
            damage_total += damage_expected;
        }
        if(attack_type == "Rapid Shot")
        {
            damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_1_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)/100.0;
            results.append("/" + QString::number(damage_expected, 'g', 4));
            damage_total += damage_expected;
        }

        if(ui->attack_2_bonus->value() > 0)
        {
            damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_2_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_2_bonus->value() + hit_bonus, enemy_ac)/100.0;
            results.append("/" + QString::number(damage_expected, 'g', 4));
            damage_total += damage_expected;
        }
        if(ui->attack_3_bonus->value() > 0)
        {
            damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_3_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_3_bonus->value() + hit_bonus, enemy_ac)/100.0;
            results.append("/" + QString::number(damage_expected, 'g', 4));
            damage_total += damage_expected;
        }
        if(ui->attack_4_bonus->value() > 0)
        {
            damage_expected = (damage_noncrit + (damage_crit - damage_noncrit) * get_critchance(ui->attack_4_bonus->value() + hit_bonus, crit_confirm_bonus, crit_min_bound, enemy_ac)/100.0) * get_hitchance(ui->attack_4_bonus->value() + hit_bonus, enemy_ac)/100.0;
            results.append("/" + QString::number(damage_expected, 'g', 4));
            damage_total += damage_expected;
        }
        results.append(" = " + QString::number(damage_total, 'g', 4) + " per turn\n");
    }
    else if (attack_type == "Cleave")
    {
        //cleave damage can only be attempted when the first target is hit
        float damage_cleave_expected = damage_expected * get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)/100.0;
        damage_total += damage_cleave_expected;
        results.append("/" + QString::number(damage_cleave_expected, 'g', 4) + " = " + QString::number(damage_total, 'g', 4) + " per turn\n");
    }
    else if (attack_type == "Great Cleave")
    {
        //theoretical maximum targets should be 8 (completely surrounding the player)
        //the rest of the maths should be like normal "Cleave"

        float hitchance = get_hitchance(ui->attack_1_bonus->value() + hit_bonus, enemy_ac)/100.0;
        float damage_cleave_expected = damage_expected;

        for(int targets_left = ui->attack_great_cleave_targets->value() - 1; targets_left > 0; --targets_left)
        {
            damage_cleave_expected *= hitchance;
            damage_total += damage_cleave_expected;
            results.append("/" + QString::number(damage_cleave_expected, 'g', 4)); //3rd target
        }

        results.append(" = " + QString::number(damage_total, 'g', 4) + " per turn (" + QString::number(ui->attack_great_cleave_targets->value()) + " targets)\n");
    }
    else if(attack_type == "Deadly Stroke")
    {
        results.append(" + 1 Con bleed per turn");
    }
    ui->result_text->setPlainText(results);
}

void MainWindow::get_warnings()
{
    QString warning = "";
    int BAB = ui->attack_1_bonus->value();

    if(ui->weapon_bonus->value() > 5)
        warning.append(" Weapon bonus must be <= 5");

    if(ui->feat_weaponfocus->checkState() && BAB < 1)
        warning.append(" Weapon Focus requires BAB >= 1");

    if(ui->feat_weaponspecialization->checkState() && BAB < 4)
        warning.append(" Weapon Specialization requires Fighter level 4");

    if(ui->feat_weaponfocus->checkState() && BAB < 8)
        warning.append(" Weapon Focus requires Fighter level 8");

    if(ui->feat_improvedcritical->checkState() && BAB < 8)
        warning.append(" Improved Critical requires BAB >= 8");

    if(ui->feat_criticalfocus->checkState() && BAB < 9)
        warning.append(" Critical Focus requires BAB >= 9");

    if(ui->feat_greaterweaponspecialization->checkState() && BAB < 12)
        warning.append(" Greater Weapon Specialization requires Fighter level 12");

    if(! warning.isEmpty())
    {
        ui->warnings->setText("Warning: " + warning);
    }
}

void MainWindow::on_feat_arcanestrike_bonus_valueChanged(int bonus)
{
    if(bonus == 0 && ui->feat_arcanestrike->checkState())
    {
        ui->feat_arcanestrike->click();
    }
    else if(! ui->feat_arcanestrike->checkState())
    {
        ui->feat_arcanestrike->click();
    }
    this->get_warnings();
}

void MainWindow::on_feat_weapontraining_bonus_valueChanged(int bonus)
{
    if(bonus == 0 && ui->feat_weapontraining->checkState())
    {
        ui->feat_weapontraining->click();
    }
    else if(! ui->feat_weapontraining->checkState())
    {
        ui->feat_weapontraining->click();
    }
    this->get_warnings();
}

void MainWindow::on_feat_weaponfocus_clicked(bool checked)
{
    if(checked == false)
    {
        if(ui->feat_greaterweaponfocus->checkState())
            ui->feat_greaterweaponfocus->click();
        if(ui->feat_weaponspecialization->checkState())
            ui->feat_weaponspecialization->click();
        if(ui->feat_greaterweaponspecialization->checkState())
            ui->feat_greaterweaponspecialization->click();
    }
    this->get_warnings();
}

void MainWindow::on_feat_greaterweaponfocus_clicked(bool checked)
{
    if(checked == true && ! ui->feat_weaponfocus->checkState())
        ui->feat_weaponfocus->click();
    this->get_warnings();
}

void MainWindow::on_feat_weaponspecialization_clicked(bool checked)
{
    if(checked == true &&! ui->feat_weaponfocus->checkState())
        ui->feat_weaponfocus->click();
    if(checked == false && ui->feat_greaterweaponspecialization->checkState())
        ui->feat_greaterweaponspecialization->click();
    this->get_warnings();
}

void MainWindow::on_feat_greaterweaponspecialization_clicked(bool checked)
{
    if(checked == true)
    {
        if(! ui->feat_weaponspecialization->checkState())
            ui->feat_weaponspecialization->click();
        if(! ui->feat_weaponfocus->checkState())
                ui->feat_weaponfocus->click();
    }
    this->get_warnings();
}

void MainWindow::on_stat_str_mod_valueChanged()
{
    this->get_warnings();
}

void MainWindow::on_weapon_flaming_clicked(bool checked)
{
    if(! checked && ui->weapon_flamingburst->checkState())
        ui->weapon_flamingburst->click();
}

void MainWindow::on_weapon_flamingburst_clicked(bool checked)
{
    if(checked && ! ui->weapon_flaming->checkState())
        ui->weapon_flaming->click();
}

void MainWindow::on_weapon_ranged_clicked(bool checked)
{
    if(checked)
    {
        ui->weapon_melee_modifiers->setProperty("visible", false);
        ui->weapon_keen->setChecked(false); ui->weapon_keen->setEnabled(false);
        ui->weapon_vicious->setChecked(false); ui->weapon_vicious->setEnabled(false);
        ui->weapon_wounding->setChecked(false); ui->weapon_wounding->setEnabled(false);

        ui->attack_type->clear();
        ui->attack_type->insertItems(0, QStringList() << "Basic Attack" << "Full Attack" << "Rapid Shot" << "Manyshot");
        ui->attack_type->setCurrentIndex(1);
    }
}

void MainWindow::on_weapon_melee_clicked(bool checked)
{
    if(checked)
    {
        ui->weapon_keen->setEnabled(true);
        ui->weapon_vicious->setEnabled(true);
        ui->weapon_wounding->setEnabled(true);

        ui->attack_type->clear();
        ui->attack_type->insertItems(0, QStringList() << "Basic Attack" << "Full Attack" << "Power Attack" << "Cleave" << "Great Cleave" <<
                                     "Vital Strike" << "Improved Vital Strike" << "Greater Vital Strike" << "Deadly Stroke");
        ui->attack_type->setCurrentIndex(1);
    }
}

void MainWindow::on_attack_type_activated(const QString &attack_type)
{
    if(attack_type == "Great Cleave")
    {
        ui->attack_great_cleave_targets_label->setVisible(true);
        ui->attack_great_cleave_targets->setVisible(true);
    }
    else
    {
        ui->attack_great_cleave_targets_label->setVisible(false);
        ui->attack_great_cleave_targets->setVisible(false);
    }
}


void MainWindow::on_about_clicked()
{
    ui->warnings->setText("Armsman was written by Russell Champoux in February 2013 and is distributed freely\nSend comments, suggestions, bug reports/fixes, or Paypal donations to RussellChamp@gmail.com");
}
