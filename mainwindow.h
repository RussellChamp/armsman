#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButton_calculate_clicked();

    void on_feat_arcanestrike_bonus_valueChanged(int arg1);

    void on_feat_weaponfocus_clicked(bool checked);

    void on_feat_greaterweaponfocus_clicked(bool checked);

    void on_feat_weaponspecialization_clicked(bool checked);

    void on_feat_greaterweaponspecialization_clicked(bool checked);

    void on_feat_weapontraining_bonus_valueChanged(int arg1);

    void on_stat_str_mod_valueChanged();

    void on_weapon_flaming_clicked(bool checked);

    void on_weapon_flamingburst_clicked(bool checked);

    void on_weapon_ranged_clicked(bool checked);

    void on_attack_type_activated(const QString &arg1);

    void on_weapon_melee_clicked(bool checked);

    void on_about_clicked();

private:
    Ui::MainWindow *ui;
    void get_warnings();
    float get_damage(bool crit);
    void calculate_stuff();
};

#endif // MAINWINDOW_H
