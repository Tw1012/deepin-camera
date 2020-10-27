#ifndef MAINWINDOWTEST_H
#define MAINWINDOWTEST_H

//#include <QTest>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QtTest/QTest>
#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>
#include <QSharedMemory>
#include <DLog>
#include <DApplicationSettings>
#include "../src/src/mainwindow.h"
#include "../src/src/capplication.h"
#include "../src/src/Settings.h"

DWIDGET_USE_NAMESPACE

class MainwindowTest : public ::testing::Test
{
public:
    MainwindowTest();
    virtual void SetUp()
    {

    }

    virtual void TearDown()
    {

    }
protected:
};

#endif // MAINWINDOWTEST_H
