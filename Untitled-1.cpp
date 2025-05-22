#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <random>
#include <fstream>
#include <thread>
#include <mutex>
#include <sstream>
#include <limits>
#include <numeric>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using json = nlohmann::json;
using namespace std;

// Forward declarations
class Asset;
class Portfolio;
class MarketDataFetcher;

// Utility functions
namespace Utils {
    // Callback function for cURL
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        try {
            s->append((char*)contents, newLength);
            return newLength;
        } catch(std::bad_alloc& e) {
            return 0;
        }
    }

    // Format currency with proper commas and decimals
    std::string formatCurrency(double amount, int precision = 2) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        
        if (amount < 0) {
            oss << "-$" << std::abs(amount);
        } else {
            oss << "$" << amount;
        }
        
        std::string result = oss.str();
        
        // Add commas for thousands
        int insertPosition = result.find('.') - 3;
        while (insertPosition > 0) {
            result.insert(insertPosition, ",");
            insertPosition -= 3;
        }
        
        return result;
    }

    // Calculate percentage change
    double percentChange(double oldValue, double newValue) {
        if (oldValue == 0) return 0.0;
        return ((newValue - oldValue) / oldValue) * 100.0;
    }

    // Generate ASCII bar chart
    std::string generateASCIIBar(double percentage, int width = 50) {
        int barWidth = static_cast<int>(percentage * width / 100.0);
        std::string bar = "[";
        for (int i = 0; i < width; ++i) {
            if (i < barWidth) {
                bar += "=";
            } else {
                bar += " ";
            }
        }
        bar += "] " + std::to_string(static_cast<int>(percentage)) + "%";
        return bar;
    }

    // Generate ASCII pie chart
    std::string generateASCIIPieChart(const std::map<std::string, double>& percentages) {
        const int chartRadius = 10;
        const int chartDiameter = chartRadius * 2 + 1;
        std::vector<std::vector<char>> chart(chartDiameter, std::vector<char>(chartDiameter, ' '));
        
        double currentAngle = 0.0;
        char symbol = 'A';
        std::map<char, std::string> legend;
        
        for (const auto& [name, percentage] : percentages) {
            double sectorAngle = percentage * 3.6; // Convert percentage to degrees (360 degrees = 100%)
            double endAngle = currentAngle + sectorAngle;
            
            for (int y = 0; y < chartDiameter; ++y) {
                for (int x = 0; x < chartDiameter; ++x) {
                    double dx = x - chartRadius;
                    double dy = chartRadius - y;
                    double distance = std::sqrt(dx*dx + dy*dy);
                    
                    if (distance <= chartRadius) {
                        double angle = std::atan2(dy, dx) * 180.0 / M_PI;
                        if (angle < 0) angle += 360;
                        
                        if (angle >= currentAngle && angle < endAngle) {
                            chart[y][x] = symbol;
                        }
                    }
                }
            }
            
            legend[symbol] = name + " (" + std::to_string(static_cast<int>(percentage)) + "%)";
            currentAngle = endAngle;
            symbol++;
        }
        
        std::string result;
        for (const auto& row : chart) {
            for (char cell : row) {
                result += cell;
            }
            result += "\n";
        }
        
        result += "\nLegend:\n";
        for (const auto& [sym, desc] : legend) {
            result += sym;
            result += " - ";
            result += desc;
            result += "\n";
        }
        
        return result;
    }
    
    // Get current date as string
    std::string getCurrentDate() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time_t);
        
        std::ostringstream oss;
        oss << std::put_time(now_tm, "%Y-%m-%d");
        return oss.str();
    }
    
    // Simulate market volatility
    double simulateVolatility(double basePrice, double volatilityFactor = 0.02) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> d(0, volatilityFactor);
        return basePrice * (1.0 + d(gen));
    }
}

// Enums for risk appetite and investment goals
enum class RiskAppetite { LOW, MEDIUM, HIGH };
enum class InvestmentGoal { WEALTH_GROWTH, STABILITY, HIGH_RETURNS };
enum class TimeHorizon { SHORT, MEDIUM, LONG };

// Class to represent a user profile
class UserProfile {
private:
    std::string name;
    int age;
    double investmentCapital;
    RiskAppetite riskAppetite;
    InvestmentGoal investmentGoal;
    TimeHorizon timeHorizon;
    double monthlyInvestment;  // For SIP

public:
    UserProfile() : age(0), investmentCapital(0.0), riskAppetite(RiskAppetite::MEDIUM),
                   investmentGoal(InvestmentGoal::WEALTH_GROWTH), timeHorizon(TimeHorizon::MEDIUM), 
                   monthlyInvestment(0.0) {}
    
    void setup() {
        std::cout << "\n========== USER PROFILE SETUP ==========\n" << std::endl;
        
        std::cout << "[Neural Scan Initiated...]" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "[Identity Verified]\n" << std::endl;
        
        std::cout << "Enter your name: ";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Clear any previous input
        std::getline(std::cin, name);
        
        std::cout << "Enter your age: ";
        std::cin >> age;
        
        std::cout << "Enter total investment capital ($): ";
        std::cin >> investmentCapital;
        
        std::cout << "Enter monthly investment amount for SIP ($): ";
        std::cin >> monthlyInvestment;
        
        int riskChoice;
        std::cout << "\nSelect your risk appetite:\n";
        std::cout << "1. Low Risk\n";
        std::cout << "2. Medium Risk\n";
        std::cout << "3. High Risk\n";
        std::cout << "Choice: ";
        std::cin >> riskChoice;
        
        switch (riskChoice) {
            case 1: riskAppetite = RiskAppetite::LOW; break;
            case 2: riskAppetite = RiskAppetite::MEDIUM; break;
            case 3: riskAppetite = RiskAppetite::HIGH; break;
            default: riskAppetite = RiskAppetite::MEDIUM;
        }
        
        int goalChoice;
        std::cout << "\nSelect your investment goal:\n";
        std::cout << "1. Wealth Growth\n";
        std::cout << "2. Stability\n";
        std::cout << "3. High Returns\n";
        std::cout << "Choice: ";
        std::cin >> goalChoice;
        
        switch (goalChoice) {
            case 1: investmentGoal = InvestmentGoal::WEALTH_GROWTH; break;
            case 2: investmentGoal = InvestmentGoal::STABILITY; break;
            case 3: investmentGoal = InvestmentGoal::HIGH_RETURNS; break;
            default: investmentGoal = InvestmentGoal::WEALTH_GROWTH;
        }
        
        int timeChoice;
        std::cout << "\nSelect your time horizon:\n";
        std::cout << "1. Short Term (1-3 years)\n";
        std::cout << "2. Medium Term (3-7 years)\n";
        std::cout << "3. Long Term (7+ years)\n";
        std::cout << "Choice: ";
        std::cin >> timeChoice;
        
        switch (timeChoice) {
            case 1: timeHorizon = TimeHorizon::SHORT; break;
            case 2: timeHorizon = TimeHorizon::MEDIUM; break;
            case 3: timeHorizon = TimeHorizon::LONG; break;
            default: timeHorizon = TimeHorizon::MEDIUM;
        }
        
        // Clear the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        std::cout << "\nProfile setup complete!\n" << std::endl;
    }
    
    // Getters
    std::string getName() const { return name; }
    int getAge() const { return age; }
    double getInvestmentCapital() const { return investmentCapital; }
    RiskAppetite getRiskAppetite() const { return riskAppetite; }
    InvestmentGoal getInvestmentGoal() const { return investmentGoal; }
    TimeHorizon getTimeHorizon() const { return timeHorizon; }
    double getMonthlyInvestment() const { return monthlyInvestment; }
    
    // Risk profile as string
    std::string getRiskProfileStr() const {
        switch (riskAppetite) {
            case RiskAppetite::LOW: return "Low Risk";
            case RiskAppetite::MEDIUM: return "Medium Risk";
            case RiskAppetite::HIGH: return "High Risk";
            default: return "Unknown";
        }
    }
    
    // Goal as string
    std::string getGoalStr() const {
        switch (investmentGoal) {
            case InvestmentGoal::WEALTH_GROWTH: return "Wealth Growth";
            case InvestmentGoal::STABILITY: return "Stability";
            case InvestmentGoal::HIGH_RETURNS: return "High Returns";
            default: return "Unknown";
        }
    }
    
    // Time horizon as string
    std::string getTimeHorizonStr() const {
        switch (timeHorizon) {
            case TimeHorizon::SHORT: return "Short Term (1-3 years)";
            case TimeHorizon::MEDIUM: return "Medium Term (3-7 years)";
            case TimeHorizon::LONG: return "Long Term (7+ years)";
            default: return "Unknown";
        }
    }
    
    void displayProfile() const {
        std::cout << "\n========== USER PROFILE ==========\n" << std::endl;
        std::cout << "Name: " << name << std::endl;
        std::cout << "Age: " << age << std::endl;
        std::cout << "Investment Capital: " << Utils::formatCurrency(investmentCapital) << std::endl;
        std::cout << "Monthly SIP Investment: " << Utils::formatCurrency(monthlyInvestment) << std::endl;
        std::cout << "Risk Appetite: " << getRiskProfileStr() << std::endl;
        std::cout << "Investment Goal: " << getGoalStr() << std::endl;
        std::cout << "Time Horizon: " << getTimeHorizonStr() << std::endl;
        std::cout << std::endl;
    }
};

// Base class for all assets
class Asset {
protected:
    std::string name;
    std::string symbol;
    double currentPrice;
    double quantity;
    double initialInvestment;
    std::vector<std::pair<std::string, double>> priceHistory;  // Date, Price
    double volatility;  // Measured as standard deviation of returns

public:
    Asset(const std::string& name, const std::string& symbol, double currentPrice, double quantity = 0.0)
        : name(name), symbol(symbol), currentPrice(currentPrice), quantity(quantity), 
          initialInvestment(currentPrice * quantity), volatility(0.0) {
        
        // Add initial price to history
        if (currentPrice > 0) {
            priceHistory.push_back({Utils::getCurrentDate(), currentPrice});
        }
    }
    
    virtual ~Asset() = default;
    
    // Getters
    std::string getName() const { return name; }
    std::string getSymbol() const { return symbol; }
    double getCurrentPrice() const { return currentPrice; }
    double getQuantity() const { return quantity; }
    double getCurrentValue() const { return currentPrice * quantity; }
    double getInitialInvestment() const { return initialInvestment; }
    double getVolatility() const { return volatility; }
    
    // Price history operations
    void addPricePoint(const std::string& date, double price) {
        priceHistory.push_back({date, price});
        updateVolatility();
    }
    
    void updateCurrentPrice(double newPrice) {
        currentPrice = newPrice;
        addPricePoint(Utils::getCurrentDate(), newPrice);
    }
    
    double getReturnPercentage() const {
        if (initialInvestment == 0) return 0.0;
        return ((getCurrentValue() - initialInvestment) / initialInvestment) * 100.0;
    }
    
    // Buy more of this asset
    void buy(double investmentAmount) {
        double additionalQuantity = investmentAmount / currentPrice;
        quantity += additionalQuantity;
        initialInvestment += investmentAmount;
    }
    
    // Sell some of this asset
    double sell(double percentageToSell) {
        if (percentageToSell <= 0 || percentageToSell > 100) {
            return 0.0;
        }
        
        double quantityToSell = quantity * (percentageToSell / 100.0);
        double saleProceeds = quantityToSell * currentPrice;
        
        quantity -= quantityToSell;
        initialInvestment *= (1.0 - percentageToSell / 100.0);
        
        return saleProceeds;
    }
    
    // Calculate volatility as standard deviation of returns
    void updateVolatility() {
        if (priceHistory.size() < 2) {
            volatility = 0.0;
            return;
        }
        
        std::vector<double> returns;
        for (size_t i = 1; i < priceHistory.size(); ++i) {
            double previousPrice = priceHistory[i-1].second;
            double currentPrice = priceHistory[i].second;
            double dailyReturn = (currentPrice - previousPrice) / previousPrice;
            returns.push_back(dailyReturn);
        }
        
        double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        
        double squaredSum = 0.0;
        for (double ret : returns) {
            squaredSum += (ret - mean) * (ret - mean);
        }
        
        volatility = std::sqrt(squaredSum / returns.size()) * 100.0; // As percentage
    }
    
    // Display asset details
    virtual void display() const {
        std::cout << name << " (" << symbol << "):" << std::endl;
        std::cout << "  Price: " << Utils::formatCurrency(currentPrice) << std::endl;
        std::cout << "  Quantity: " << std::fixed << std::setprecision(6) << quantity << std::endl;
        std::cout << "  Current Value: " << Utils::formatCurrency(getCurrentValue()) << std::endl;
        std::cout << "  Initial Investment: " << Utils::formatCurrency(initialInvestment) << std::endl;
        std::cout << "  Return: " << std::fixed << std::setprecision(2) << getReturnPercentage() << "%" << std::endl;
        std::cout << "  Volatility: " << std::fixed << std::setprecision(2) << volatility << "%" << std::endl;
        std::cout << std::endl;
    }
    
    // Each asset type might have unique analysis methods
    virtual std::string getAnalysis() const {
        std::string analysis = "Analysis for " + name + " (" + symbol + "):\n";
        
        if (priceHistory.size() >= 2) {
            double oldestPrice = priceHistory.front().second;
            double latestPrice = priceHistory.back().second;
            double priceChange = Utils::percentChange(oldestPrice, latestPrice);
            
            analysis += "  Price change since tracking: " + std::to_string(priceChange) + "%\n";
            
            if (priceChange > 0) {
                analysis += "  The price has increased since tracking began.\n";
            } else if (priceChange < 0) {
                analysis += "  The price has decreased since tracking began.\n";
            } else {
                analysis += "  The price remains stable since tracking began.\n";
            }
        }
        
        if (volatility < 5.0) {
            analysis += "  Low volatility: This asset has been stable recently.\n";
        } else if (volatility < 15.0) {
            analysis += "  Medium volatility: This asset shows moderate price movements.\n";
        } else {
            analysis += "  High volatility: This asset has significant price fluctuations.\n";
        }
        
        return analysis;
    }
};

// SIP (Systematic Investment Plan) - Mutual Funds or Index funds
class SIP : public Asset {
private:
    double expectedAnnualReturn;
    std::string fundType; // Index, Equity, Debt, etc.
    double expenseRatio;

public:
    SIP(const std::string& name, const std::string& symbol, double currentPrice, 
        double quantity = 0.0, double expectedAnnualReturn = 12.0, 
        const std::string& fundType = "Index", double expenseRatio = 0.5)
        : Asset(name, symbol, currentPrice, quantity),
          expectedAnnualReturn(expectedAnnualReturn), fundType(fundType), expenseRatio(expenseRatio) {}
    
    // Getters
    double getExpectedAnnualReturn() const { return expectedAnnualReturn; }
    std::string getFundType() const { return fundType; }
    double getExpenseRatio() const { return expenseRatio; }
    
    // Project growth over years
    double projectGrowth(int years, double monthlyContribution = 0.0) const {
        double monthlyRate = (expectedAnnualReturn / 100.0) / 12.0;
        double totalMonths = years * 12.0;
        double currentValue = getCurrentValue();
        
        // Formula for future value with regular contributions
        double futureValue = currentValue * std::pow(1 + monthlyRate, totalMonths);
        
        if (monthlyContribution > 0) {
            futureValue += monthlyContribution * ((std::pow(1 + monthlyRate, totalMonths) - 1) / monthlyRate);
        }
        
        return futureValue;
    }
    
    // Override display to include SIP-specific details
    void display() const override {
        Asset::display();
        std::cout << "  Fund Type: " << fundType << std::endl;
        std::cout << "  Expected Annual Return: " << expectedAnnualReturn << "%" << std::endl;
        std::cout << "  Expense Ratio: " << expenseRatio << "%" << std::endl;
        
        // Show projected growth
        std::cout << "  Projected Value (3 years): " << Utils::formatCurrency(projectGrowth(3, 0)) << std::endl;
        std::cout << "  Projected Value (5 years): " << Utils::formatCurrency(projectGrowth(5, 0)) << std::endl;
        std::cout << "  Projected Value (10 years): " << Utils::formatCurrency(projectGrowth(10, 0)) << std::endl;
        std::cout << std::endl;
    }
    
    // Override get analysis to include SIP-specific analysis
    std::string getAnalysis() const override {
        std::string analysis = Asset::getAnalysis();
        
        analysis += "  This is a " + fundType + " fund with an expense ratio of " + 
                    std::to_string(expenseRatio) + "%.\n";
        
        if (expenseRatio > 1.0) {
            analysis += "  The expense ratio is relatively high. Consider lower-cost alternatives.\n";
        } else {
            analysis += "  The expense ratio is reasonable for this type of fund.\n";
        }
        
        if (expectedAnnualReturn > 15.0) {
            analysis += "  The expected return seems optimistic. Be prepared for potential underperformance.\n";
        }
        
        return analysis;
    }
};

// Forex trading asset
class Forex : public Asset {
private:
    std::string baseCurrency;
    std::string quoteCurrency;
    double spreadPercentage;
    std::string trend; // "Bullish", "Bearish", "Neutral"

public:
    Forex(const std::string& name, const std::string& symbol, double currentPrice, 
          const std::string& baseCurrency, const std::string& quoteCurrency,
          double quantity = 0.0, double spreadPercentage = 0.1)
        : Asset(name, symbol, currentPrice, quantity),
          baseCurrency(baseCurrency), quoteCurrency(quoteCurrency),
          spreadPercentage(spreadPercentage), trend("Neutral") {}
    
    // Getters
    std::string getBaseCurrency() const { return baseCurrency; }
    std::string getQuoteCurrency() const { return quoteCurrency; }
    double getSpreadPercentage() const { return spreadPercentage; }
    std::string getTrend() const { return trend; }
    
    // Set trend based on price movement
    void updateTrend() {
        if (priceHistory.size() < 5) {
            trend = "Neutral";
            return;
        }
        
        // Calculate average price over last 5 data points
        double sum = 0.0;
        for (int i = priceHistory.size() - 5; i < priceHistory.size(); ++i) {
            sum += priceHistory[i].second;
        }
        double avg = sum / 5.0;
        
        // Compare with current price
        if (currentPrice > avg * 1.02) {
            trend = "Bullish";
        } else if (currentPrice < avg * 0.98) {
            trend = "Bearish";
        } else {
            trend = "Neutral";
        }
    }
    
    // Override price update to also update trend
    void updateCurrentPrice(double newPrice) {
        Asset::updateCurrentPrice(newPrice);
        updateTrend();
    }
    
    // Override display for Forex-specific details
    void display() const override {
        Asset::display();
        std::cout << "  Pair: " << baseCurrency << "/" << quoteCurrency << std::endl;
        std::cout << "  Spread: " << spreadPercentage << "%" << std::endl;
        std::cout << "  Current Trend: " << trend << std::endl;
        std::cout << std::endl;
    }
    
    // Override get analysis for Forex-specific analysis
    std::string getAnalysis() const override {
        std::string analysis = Asset::getAnalysis();
        
        analysis += "  This forex pair (" + baseCurrency + "/" + quoteCurrency + ") ";
        
        if (trend == "Bullish") {
            analysis += "is in an uptrend. Consider taking profit or trailing stops.\n";
        } else if (trend == "Bearish") {
            analysis += "is in a downtrend. Consider hedging or reducing exposure.\n";
        } else {
            analysis += "is in a neutral trend. Monitor for breakout opportunities.\n";
        }
        
        if (volatility > 10.0) {
            analysis += "  High volatility in this pair suggests caution with position sizing.\n";
        }
        
        return analysis;
    }
};

// Cryptocurrency asset
class Cryptocurrency : public Asset {
private:
    double marketCap;
    std::string networkStatus; // "Healthy", "Congested", etc.
    bool isStaking;
    double stakingYield;

public:
    Cryptocurrency(const std::string& name, const std::string& symbol, double currentPrice, 
                  double marketCap, double quantity = 0.0, bool isStaking = false, 
                  double stakingYield = 0.0)
        : Asset(name, symbol, currentPrice, quantity),
          marketCap(marketCap), networkStatus("Healthy"),
          isStaking(isStaking), stakingYield(stakingYield) {}
    
    // Getters
    double getMarketCap() const { return marketCap; }
    std::string getNetworkStatus() const { return networkStatus; }
    bool getIsStaking() const { return isStaking; }
    double getStakingYield() const { return stakingYield; }
    
    // Update market cap based on new price
    void updateMarketCap() {
        // Simplified - in reality would need to know total supply
        double ratio = currentPrice / priceHistory.front().second;
        marketCap *= ratio;
    }
    
    // Enable staking
    void enableStaking(double yield) {
        isStaking = true;
        stakingYield = yield;
    }
    
    // Disable staking
    void disableStaking() {
        isStaking = false;
        stakingYield = 0.0;
    }
    
    // Calculate staking rewards
    double calculateStakingRewards(int days) const {
        if (!isStaking || stakingYield <= 0) return 0.0;
        
        double dailyRate = stakingYield / 365.0;
        return getCurrentValue() * (std::pow(1 + dailyRate / 100.0, days) - 1);
    }
    
    // Override display for crypto-specific details
    void display() const override {
        Asset::display();
        std::cout << "  Market Cap: " << Utils::formatCurrency(marketCap) << std::endl;
        std::cout << "  Network Status: " << networkStatus << std::endl;
        
        if (isStaking) {
            std::cout << "  Staking Enabled: Yes" << std::endl;
            std::cout << "  Staking Yield: " << stakingYield << "% APY" << std::endl;
            std::cout << "  Projected Staking Reward (30 days): " 
                      << Utils::formatCurrency(calculateStakingRewards(30)) << std::endl;
        } else {
            std::cout << "  Staking Enabled: No" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Override get analysis for crypto-specific analysis
    std::string getAnalysis() const override {
        std::string analysis = Asset::getAnalysis();
        
        analysis += "  " + name + " has a market cap of " + Utils::formatCurrency(marketCap) + ".\n";
        
        if (volatility > 20.0) {
            analysis += "  This cryptocurrency shows extreme volatility. Consider reducing exposure.\n";
        }
        
        if (isStaking) {
            analysis += "  You are earning " + std::to_string(stakingYield) + 
                       "% APY through staking, which helps offset volatility.\n";
        } else {
            analysis += "  Consider staking options to earn passive income from your holdings.\n";
        }
        
        return analysis;
    }
};

// Commodity (Gold) asset
class Commodity : public Asset {
private:
    std::string grade; // e.g., "24K", "22K" for gold
    bool isPhysical;   // physical gold vs paper gold (ETF)

public:
    Commodity(const std::string& name, const std::string& symbol, double currentPrice,
             const std::string& grade = "24K", bool isPhysical = false, double quantity = 0.0)
        : Asset(name, symbol, currentPrice, quantity),
          grade(grade), isPhysical(isPhysical) {}
    
    // Getters
    std::string getGrade() const { return grade; }
    bool getIsPhysical() const { return isPhysical; }
    
    // Inflation hedge calculation
    double calculateInflationHedge(double inflationRate, int years) const {
        double annualLossToInflation = inflationRate / 100.0;
        double valueWithoutHedge = getCurrentValue() * std::pow(1 - annualLossToInflation, years);
        return getCurrentValue() - valueWithoutHedge;
    }
    
    // Override display for commodity-specific details
    void display() const override {
        Asset::display();
        std::cout << "  Grade: " << grade << std::endl;
        std::cout << "  Physical Holding: " << (isPhysical ? "Yes" : "No") << std::endl;
        std::cout << "  Inflation Hedge (5% inflation, 5 years): " 
                  << Utils::formatCurrency(calculateInflationHedge(5.0, 5)) << std::endl;
        std::cout << std::endl;
    }
    
    // Override get analysis for commodity-specific analysis
    std::string getAnalysis() const override {
        std::string analysis = Asset::getAnalysis();
        
        analysis += "  This " + grade + " gold is held as " + 
                   (isPhysical ? "physical metal" : "a paper investment") + ".\n";
        
        if (volatility < 10.0) {
            analysis += "  Gold is currently showing relative stability, providing a good hedge.\n";
        } else {
            analysis += "  Gold is showing higher than usual volatility. Monitor global macro events.\n";
        }
        
        return analysis;
    }
};

// Fiat Currency asset (e.g., USD)
class FiatCurrency : public Asset {
private:
    std::string country;
    double interestRate;     // Current interest rate in the country
    double inflationRate;    // Current inflation rate

public:
    FiatCurrency(const std::string& name, const std::string& symbol, double currentPrice,
                const std::string& country, double interestRate = 0.0, double inflationRate = 0.0,
                double quantity = 0.0)
        : Asset(name, symbol, currentPrice, quantity),
          country(country), interestRate(interestRate), inflationRate(inflationRate) {}
    
    // Getters
    std::string getCountry() const { return country; }
    double getInterestRate() const { return interestRate; }
    double getInflationRate() const { return inflationRate; }
    
    // Calculate real return (accounting for inflation)
    double getRealReturn() const {
        return interestRate - inflationRate;
    }
    
    // Calculate purchasing power after a period
    double calculatePurchasingPower(int years) const {
        double realReturnRate = getRealReturn() / 100.0;
        return getCurrentValue() * std::pow(1 + realReturnRate, years);
    }
    
    // Override display for fiat-specific details
    void display() const override {
        Asset::display();
        std::cout << "  Country: " << country << std::endl;
        std::cout << "  Interest Rate: " << interestRate << "%" << std::endl;
        std::cout << "  Inflation Rate: " << inflationRate << "%" << std::endl;
        std::cout << "  Real Return: " << getRealReturn() << "%" << std::endl;
        std::cout << "  Purchasing Power (5 years): " 
                  << Utils::formatCurrency(calculatePurchasingPower(5)) << std::endl;
        std::cout << std::endl;
    }
    
    // Override get analysis for fiat-specific analysis
    std::string getAnalysis() const override {
        std::string analysis = Asset::getAnalysis();
        
        analysis += "  " + name + " has an interest rate of " + std::to_string(interestRate) + 
                   "% and inflation of " + std::to_string(inflationRate) + "%.\n";
        
        double realReturn = getRealReturn();
        if (realReturn < 0) {
            analysis += "  This currency has a negative real return, losing purchasing power over time.\n";
            analysis += "  Consider alternatives for long-term holdings.\n";
        } else if (realReturn < 1.0) {
            analysis += "  This currency is barely maintaining purchasing power.\n";
        } else {
            analysis += "  This currency has a positive real return, which is favorable.\n";
        }
        
        return analysis;
    }
};

// Market Data Fetcher class to get live market data
class MarketDataFetcher {
private:
    std::string apiKey;
    std::map<std::string, double> lastFetchedPrices;
    std::mutex priceMutex;

    // Initialize cURL
    CURL* initCurl() {
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        return curl;
    }

    // Fetch data from API
    std::string fetchFromAPI(const std::string& url) {
        CURL* curl = initCurl();
        std::string responseData;
        
        if (!curl) {
            std::cerr << "Failed to initialize cURL" << std::endl;
            return "";
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return "";
        }
        
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            std::cerr << "HTTP error: " << http_code << std::endl;
            curl_easy_cleanup(curl);
            return "";
        }
        
        curl_easy_cleanup(curl);
        return responseData;
    }
    
    // Parse JSON for asset price
    double extractPriceFromJSON(const std::string& jsonStr, const std::string& assetSymbol) {
        try {
            json j = json::parse(jsonStr);
            
            // Different APIs might have different JSON structures
            // This is a simplified example - real implementation would need to handle various API formats
            if (j.contains("price")) {
                return j["price"].get<double>();
            } else if (j.contains("rates") && j["rates"].contains(assetSymbol)) {
                return j["rates"][assetSymbol].get<double>();
            } else if (j.contains("data") && j["data"].contains("last")) {
                return j["data"]["last"].get<double>();
            } else if (j.contains("ticker") && j["ticker"].contains("price")) {
                return j["ticker"]["price"].get<double>();
            } else {
                // For demonstration, return a simulated price if we can't find it in JSON
                return simulatePrice(assetSymbol);
            }
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            return simulatePrice(assetSymbol);
        }
    }
    
    // Simulate price for demonstration or when API fails
    double simulatePrice(const std::string& symbol) {
        // Base prices for simulation
        std::map<std::string, double> basePrices = {
            {"BTC", 40000.0},
            {"ETH", 2000.0},
            {"EUR/USD", 1.10},
            {"USD/INR", 75.0},
            {"GBP/USD", 1.35},
            {"XAU/USD", 1800.0},
            {"USD", 1.0},
            {"VTI", 200.0},
            {"VOO", 380.0}
        };
        
        // If we already have a price for this symbol, apply some random variation
        std::lock_guard<std::mutex> lock(priceMutex);
        if (lastFetchedPrices.find(symbol) != lastFetchedPrices.end()) {
            double lastPrice = lastFetchedPrices[symbol];
            double newPrice = Utils::simulateVolatility(lastPrice);
            lastFetchedPrices[symbol] = newPrice;
            return newPrice;
        }
        
        // Otherwise use base price or generate one
        double basePrice = (basePrices.find(symbol) != basePrices.end()) ? 
                           basePrices[symbol] : 100.0;
        double newPrice = Utils::simulateVolatility(basePrice);
        lastFetchedPrices[symbol] = newPrice;
        return newPrice;
    }

public:
    MarketDataFetcher(const std::string& apiKey = "") : apiKey(apiKey) {
        // Initialize cURL globally
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~MarketDataFetcher() {
        // Clean up cURL
        curl_global_cleanup();
    }
    
    // Get price for a specific asset
    double getPrice(const std::string& symbol, bool useRealAPI = false) {
        if (useRealAPI) {
            // Use different APIs based on asset type
            std::string apiUrl;
            
            if (symbol == "BTC" || symbol == "ETH") {
                // Cryptocurrency API (CoinGecko example)
                apiUrl = "https://api.coingecko.com/api/v3/simple/price?ids=" + 
                         (symbol == "BTC" ? "bitcoin" : "ethereum") + 
                         "&vs_currencies=usd";
            } else if (symbol == "XAU/USD") {
                // Gold price API
                apiUrl = "https://forex-data-feed.swissquote.com/public-quotes/bboquotes/instrument/XAU/USD";
            } else if (symbol.find('/') != std::string::npos) {
                // Forex API
                apiUrl = "https://api.exchangerate-api.com/v4/latest/" + symbol.substr(0, 3);
            } else {
                // Stock/ETF API
                apiUrl = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + apiKey;
            }
            
            std::string response = fetchFromAPI(apiUrl);
            return extractPriceFromJSON(response, symbol);
        } else {
            // Use simulated prices for demonstration
            return simulatePrice(symbol);
        }
    }
    
    // Update multiple prices at once
    std::map<std::string, double> updatePrices(const std::vector<std::string>& symbols, bool useRealAPI = false) {
        std::map<std::string, double> updatedPrices;
        
        for (const auto& symbol : symbols) {
            double price = getPrice(symbol, useRealAPI);
            
            std::lock_guard<std::mutex> lock(priceMutex);
            lastFetchedPrices[symbol] = price;
            updatedPrices[symbol] = price;
        }
        
        return updatedPrices;
    }
    
    // Get volatility index (VIX) - simulated
    double getVIX() {
        return simulatePrice("VIX");
    }
    
    // Get economic indicators - simulated
    double getInflationRate(const std::string& country = "US") {
        // Simulated inflation rates
        std::map<std::string, double> inflationRates = {
            {"US", 2.5},
            {"EU", 2.0},
            {"UK", 3.0},
            {"IN", 5.5},
            {"JP", 0.5}
        };
        
        return inflationRates.find(country) != inflationRates.end() ? 
               inflationRates[country] : 2.0;
    }
    
    // Get interest rate - simulated
    double getInterestRate(const std::string& country = "US") {
        // Simulated interest rates
        std::map<std::string, double> interestRates = {
            {"US", 0.5},
            {"EU", 0.0},
            {"UK", 0.75},
            {"IN", 4.5},
            {"JP", -0.1}
        };
        
        return interestRates.find(country) != interestRates.end() ? 
               interestRates[country] : 0.5;
    }
};

// SIP Manager to handle systematic investment plans
class SIPManager {
private:
    double monthlyAmount;
    std::map<std::string, double> allocation; // Asset symbol to percentage allocation
    std::chrono::system_clock::time_point lastInvestmentDate;
    bool autoInvest;

public:
    SIPManager(double monthlyAmount = 0.0, bool autoInvest = true)
        : monthlyAmount(monthlyAmount), autoInvest(autoInvest) {
        // Initialize last investment date to current time
        lastInvestmentDate = std::chrono::system_clock::now();
    }
    
    // Set monthly investment amount
    void setMonthlyAmount(double amount) {
        monthlyAmount = amount;
    }
    
    // Set allocation percentages
    void setAllocation(const std::map<std::string, double>& newAllocation) {
        // Validate that percentages add up to 100%
        double total = 0.0;
        for (const auto& [symbol, percentage] : newAllocation) {
            total += percentage;
        }
        
        if (std::abs(total - 100.0) > 0.01) {
            std::cerr << "Warning: Allocation percentages do not add up to 100%. Adjusting..." << std::endl;
            
            // Adjust proportionally
            allocation.clear();
            for (const auto& [symbol, percentage] : newAllocation) {
                allocation[symbol] = (percentage / total) * 100.0;
            }
        } else {
            allocation = newAllocation;
        }
    }
    
    // Check if it's time for the next investment
    bool isTimeForInvestment() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - lastInvestmentDate);
        
        // Simplified: Consider a month as 30 days
        return duration.count() >= 30 * 24;
    }
    
    // Execute the monthly investment
    std::map<std::string, double> executeInvestment(bool force = false) {
        if (!force && !isTimeForInvestment()) {
            return {};
        }
        
        std::map<std::string, double> investments;
        
        for (const auto& [symbol, percentage] : allocation) {
            double amount = monthlyAmount * (percentage / 100.0);
            investments[symbol] = amount;
        }
        
        // Update last investment date
        lastInvestmentDate = std::chrono::system_clock::now();
        
        return investments;
    }
    
    // Simulate multiple months of investments
    std::map<std::string, std::vector<double>> simulateInvestments(int months) {
        std::map<std::string, std::vector<double>> simulatedInvestments;
        
        for (int i = 0; i < months; ++i) {
            auto monthlyInvestments = executeInvestment(true);
            
            for (const auto& [symbol, amount] : monthlyInvestments) {
                simulatedInvestments[symbol].push_back(amount);
            }
        }
        
        return simulatedInvestments;
    }
    
    // Calculate projected growth of SIP over time
    double calculateProjectedGrowth(int months, double annualReturnRate) {
        double monthlyRate = annualReturnRate / 12.0 / 100.0;
        double futureValue = 0.0;
        
        // Using SIP compound interest formula: P * ((1 + r)^n - 1) / r * (1 + r)
        futureValue = monthlyAmount * ((std::pow(1 + monthlyRate, months) - 1) / monthlyRate) * (1 + monthlyRate);
        
        return futureValue;
    }
    
    // Get allocation
    const std::map<std::string, double>& getAllocation() const {
        return allocation;
    }
    
    // Get monthly amount
    double getMonthlyAmount() const {
        return monthlyAmount;
    }
    
    // Toggle auto-invest
    void toggleAutoInvest() {
        autoInvest = !autoInvest;
    }
    
    // Get auto-invest status
    bool getAutoInvestStatus() const {
        return autoInvest;
    }
    
    // Display SIP details
    void display() const {
        std::cout << "\n========== SIP MANAGER ==========\n" << std::endl;
        std::cout << "Monthly Investment: " << Utils::formatCurrency(monthlyAmount) << std::endl;
        std::cout << "Auto-Invest: " << (autoInvest ? "Enabled" : "Disabled") << std::endl;
        std::cout << "Current Allocation:" << std::endl;
        
        for (const auto& [symbol, percentage] : allocation) {
            std::cout << "  " << symbol << ": " << percentage << "%" << std::endl;
        }
        
        std::cout << "\nProjected Growth (10% annual return):" << std::endl;
        std::cout << "  1 Year: " << Utils::formatCurrency(calculateProjectedGrowth(12, 10.0)) << std::endl;
        std::cout << "  5 Years: " << Utils::formatCurrency(calculateProjectedGrowth(60, 10.0)) << std::endl;
        std::cout << "  10 Years: " << Utils::formatCurrency(calculateProjectedGrowth(120, 10.0)) << std::endl;
        std::cout << "  20 Years: " << Utils::formatCurrency(calculateProjectedGrowth(240, 10.0)) << std::endl;
        
        std::cout << std::endl;
    }
};

// Risk Analyzer class to assess portfolio risk
class RiskAnalyzer {
private:
    double riskScore; // 0-100 (0 = lowest risk, 100 = highest risk)
    std::map<std::string, double> idealAllocation; // Based on risk score
    double volatilityThreshold; // Threshold for considering an asset volatile

public:
    RiskAnalyzer(double riskScore = 50.0, double volatilityThreshold = 15.0)
        : riskScore(riskScore), volatilityThreshold(volatilityThreshold) {
        updateIdealAllocation();
    }
    
    // Set risk score and update ideal allocation
    void setRiskScore(double newRiskScore) {
        riskScore = std::min(100.0, std::max(0.0, newRiskScore));
        updateIdealAllocation();
    }
    
    // Get risk score
    double getRiskScore() const {
        return riskScore;
    }
    
    // Update ideal allocation based on risk score
    void updateIdealAllocation() {
        // Clear previous allocation
        idealAllocation.clear();
        
        if (riskScore < 30.0) {
            // Low risk
            idealAllocation["SIP"] = 60.0;
            idealAllocation["USD"] = 20.0;
            idealAllocation["XAU/USD"] = 10.0;
            idealAllocation["EUR/USD"] = 5.0;
            idealAllocation["BTC"] = 5.0;
        } else if (riskScore < 70.0) {
            // Medium risk
            idealAllocation["SIP"] = 40.0;
            idealAllocation["EUR/USD"] = 20.0;
            idealAllocation["BTC"] = 15.0;
            idealAllocation["XAU/USD"] = 15.0;
            idealAllocation["USD"] = 10.0;
        } else {
            // High risk
            idealAllocation["SIP"] = 20.0;
            idealAllocation["EUR/USD"] = 30.0;
            idealAllocation["BTC"] = 30.0;
            idealAllocation["XAU/USD"] = 10.0;
            idealAllocation["USD"] = 10.0;
        }
    }
    
    // Get ideal allocation
    const std::map<std::string, double>& getIdealAllocation() const {
        return idealAllocation;
    }
    
    // Calculate portfolio volatility
    double calculatePortfolioVolatility(const std::map<std::string, std::shared_ptr<Asset>>& assets) {
        double totalValue = 0.0;
        double weightedVolatility = 0.0;
        
        // Calculate total portfolio value
        for (const auto& [symbol, asset] : assets) {
            totalValue += asset->getCurrentValue();
        }
        
        if (totalValue <= 0.0) return 0.0;
        
        // Calculate weighted volatility
        for (const auto& [symbol, asset] : assets) {
            double weight = asset->getCurrentValue() / totalValue;
            weightedVolatility += weight * asset->getVolatility();
        }
        
        return weightedVolatility;
    }
    
    // Assess if an asset is too volatile
    bool isAssetTooVolatile(const Asset& asset) const {
        return asset.getVolatility() > volatilityThreshold;
    }
    
    // Calculate risk-adjusted return (Sharpe Ratio-like)
    double calculateRiskAdjustedReturn(const std::map<std::string, std::shared_ptr<Asset>>& assets, double riskFreeRate = 0.5) {
        double totalValue = 0.0;
        double weightedReturn = 0.0;
        
        // Calculate total portfolio value and weighted return
        for (const auto& [symbol, asset] : assets) {
            totalValue += asset->getCurrentValue();
        }
        
        if (totalValue <= 0.0) return 0.0;
        
        for (const auto& [symbol, asset] : assets) {
            double weight = asset->getCurrentValue() / totalValue;
            weightedReturn += weight * asset->getReturnPercentage();
        }
        
        double portfolioVolatility = calculatePortfolioVolatility(assets);
        
        // Avoid division by zero
        if (portfolioVolatility <= 0.0) return 0.0;
        
        // (Portfolio Return - Risk-Free Rate) / Portfolio Volatility
        return (weightedReturn - riskFreeRate) / portfolioVolatility;
    }
    
    // Recommend rebalancing based on current allocation vs ideal
    std::map<std::string, double> recommendRebalancing(const std::map<std::string, std::shared_ptr<Asset>>& assets) {
        std::map<std::string, double> currentAllocation;
        std::map<std::string, double> recommendations;
        double totalValue = 0.0;
        
        // Calculate total portfolio value
        for (const auto& [symbol, asset] : assets) {
            totalValue += asset->getCurrentValue();
        }
        
        if (totalValue <= 0.0) return recommendations;
        
        // Calculate current allocation percentages
        for (const auto& [symbol, asset] : assets) {
            currentAllocation[symbol] = (asset->getCurrentValue() / totalValue) * 100.0;
        }
        
        // Compare with ideal allocation and generate recommendations
        for (const auto& [symbol, idealPercent] : idealAllocation) {
            double currentPercent = (currentAllocation.find(symbol) != currentAllocation.end()) ? 
                                   currentAllocation[symbol] : 0.0;
            
            double difference = idealPercent - currentPercent;
            
            // Only recommend significant changes (more than 5% difference)
            if (std::abs(difference) >= 5.0) {
                recommendations[symbol] = difference;
            }
        }
        
        return recommendations;
    }
    
    // Adjust risk score based on market conditions
    void adjustRiskScoreForMarketConditions(double vix, double btcVolatility) {
        double adjustment = 0.0;
        
        // VIX adjustment
        if (vix > 30.0) {
            // High market volatility, reduce risk
            adjustment -= 10.0;
        } else if (vix < 15.0) {
            // Low market volatility, can increase risk
            adjustment += 5.0;
        }
        
        // BTC volatility adjustment
        if (btcVolatility > 20.0) {
            // High crypto volatility, reduce risk
            adjustment -= 5.0;
        }
        
        // Apply adjustment with limits
        double newRiskScore = riskScore + adjustment;
        setRiskScore(std::min(100.0, std::max(0.0, newRiskScore)));
    }
    
    // Display risk analysis
    void display() const {
        std::cout << "\n========== RISK ANALYSIS ==========\n" << std::endl;
        std::cout << "Risk Score: " << riskScore << "/100" << std::endl;
        std::cout << "Risk Profile: " << getRiskProfileStr() << std::endl;
        std::cout << "Volatility Threshold: " << volatilityThreshold << "%" << std::endl;
        
        std::cout << "\nIdeal Asset Allocation:" << std::endl;
        for (const auto& [symbol, percentage] : idealAllocation) {
            std::cout << "  " << symbol << ": " << percentage << "%" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Get risk profile as string
    std::string getRiskProfileStr() const {
        if (riskScore < 30.0) {
            return "Conservative";
        } else if (riskScore < 70.0) {
            return "Moderate";
        } else {
            return "Aggressive";
        }
    }
};

// Portfolio Manager class to manage all assets
class PortfolioManager {
private:
    std::map<std::string, std::shared_ptr<Asset>> assets;
    RiskAnalyzer riskAnalyzer;
    MarketDataFetcher dataFetcher;
    SIPManager sipManager;
    std::vector<std::pair<std::string, double>> historicalValues; // Date, Total Value
    double initialInvestment;
    std::string lastRebalanceDate;

public:
    PortfolioManager(const UserProfile& userProfile)
        : riskAnalyzer(convertRiskAppetiteToScore(userProfile.getRiskAppetite())),
          sipManager(userProfile.getMonthlyInvestment()),
          initialInvestment(userProfile.getInvestmentCapital()) {
        
        // Set up SIP allocation based on risk profile
        setupInitialAllocation(userProfile);
        
        // Add initial capital entry to historical values
        historicalValues.push_back({Utils::getCurrentDate(), initialInvestment});
    }
    
    // Add a new asset to the portfolio
    void addAsset(const std::string& symbol, std::shared_ptr<Asset> asset) {
        assets[symbol] = asset;
    }
    
    // Remove an asset from the portfolio
    bool removeAsset(const std::string& symbol) {
        if (assets.find(symbol) != assets.end()) {
            assets.erase(symbol);
            return true;
        }
        return false;
    }
    
    // Get an asset by symbol
    std::shared_ptr<Asset> getAsset(const std::string& symbol) {
        if (assets.find(symbol) != assets.end()) {
            return assets[symbol];
        }
        return nullptr;
    }
    
    // Convert RiskAppetite enum to numerical score
    double convertRiskAppetiteToScore(RiskAppetite appetite) {
        switch (appetite) {
            case RiskAppetite::LOW: return 25.0;
            case RiskAppetite::MEDIUM: return 50.0;
            case RiskAppetite::HIGH: return 75.0;
            default: return 50.0;
        }
    }
    
    // Set up initial allocation based on user profile
    void setupInitialAllocation(const UserProfile& userProfile) {
        std::map<std::string, double> allocation;
        
        switch (userProfile.getRiskAppetite()) {
            case RiskAppetite::LOW:
                allocation["SIP"] = 60.0;
                allocation["USD"] = 20.0;
                allocation["XAU/USD"] = 10.0;
                allocation["EUR/USD"] = 5.0;
                allocation["BTC"] = 5.0;
                break;
                
            case RiskAppetite::MEDIUM:
                allocation["SIP"] = 40.0;
                allocation["EUR/USD"] = 20.0;
                allocation["BTC"] = 15.0;
                allocation["XAU/USD"] = 15.0;
                allocation["USD"] = 10.0;
                break;
                
            case RiskAppetite::HIGH:
                allocation["SIP"] = 20.0;
                allocation["EUR/USD"] = 30.0;
                allocation["BTC"] = 30.0;
                allocation["XAU/USD"] = 10.0;
                allocation["USD"] = 10.0;
                break;
        }
        
        // Set the allocation for SIP manager
        sipManager.setAllocation(allocation);
        
        // Also update the risk analyzer's ideal allocation
        riskAnalyzer.setRiskScore(convertRiskAppetiteToScore(userProfile.getRiskAppetite()));
    }
    
    // Initialize the portfolio with the given capital based on allocation
    void initializePortfolio(double capital) {
        // Get allocation from SIP manager
        const auto& allocation = sipManager.getAllocation();
        
        // Initialize assets with allocated capital
        for (const auto& [symbol, percentage] : allocation) {
            double amount = capital * (percentage / 100.0);
            
            // Fetch current price
            double price = dataFetcher.getPrice(symbol);
            
            // Create appropriate asset type based on symbol
            std::shared_ptr<Asset> asset;
            
            double quantity = amount / price;
            
            if (symbol == "SIP") {
                // Index ETF as SIP
                asset = std::make_shared<SIP>("Vanguard Total Stock Market ETF", "VTI", price, quantity);
            } else if (symbol == "BTC") {
                // Cryptocurrency
                asset = std::make_shared<Cryptocurrency>("Bitcoin", "BTC", price, 1000000000000.0, quantity);
            } else if (symbol == "XAU/USD") {
                // Gold
                asset = std::make_shared<Commodity>("Gold", "XAU/USD", price, "24K", false, quantity);
            } else if (symbol == "USD") {
                // USD as cash
                double interestRate = dataFetcher.getInterestRate("US");
                double inflationRate = dataFetcher.getInflationRate("US");
                asset = std::make_shared<FiatCurrency>("US Dollar", "USD", 1.0, "United States", 
                                                      interestRate, inflationRate, amount);
            } else if (symbol.find('/') != std::string::npos) {
                // Forex
                std::string baseCurrency = symbol.substr(0, 3);
                std::string quoteCurrency = symbol.substr(4, 3);
                asset = std::make_shared<Forex>(baseCurrency + " to " + quoteCurrency, symbol, 
                                               price, baseCurrency, quoteCurrency, quantity);
            } else {
                // Generic asset as fallback
                asset = std::make_shared<Asset>(symbol, symbol, price, quantity);
            }
            
            addAsset(symbol, asset);
        }
        
        // Record initial portfolio value
        recordPortfolioValue();
    }
    
    // Update asset prices with latest market data
    void updatePrices(bool useRealAPI = false) {
        std::vector<std::string> symbols;
        
        for (const auto& [symbol, asset] : assets) {
            symbols.push_back(symbol);
        }
        
        std::map<std::string, double> newPrices = dataFetcher.updatePrices(symbols, useRealAPI);
        
        for (const auto& [symbol, price] : newPrices) {
            if (assets.find(symbol) != assets.end()) {
                assets[symbol]->updateCurrentPrice(price);
            }
        }
        
        recordPortfolioValue();
    }
    
    // Execute SIP investments
    void executeSIPInvestment(bool force = false) {
        if (!sipManager.getAutoInvestStatus() && !force) {
            return;
        }
        
        std::map<std::string, double> investments = sipManager.executeInvestment(force);
        
        for (const auto& [symbol, amount] : investments) {
            if (assets.find(symbol) != assets.end() && amount > 0) {
                assets[symbol]->buy(amount);
                std::cout << "SIP Investment: Bought " << Utils::formatCurrency(amount) 
                          << " worth of " << symbol << std::endl;
            }
        }
        
        recordPortfolioValue();
    }
    
    // Record current portfolio value for historical tracking
    void recordPortfolioValue() {
        double totalValue = getTotalValue();
        historicalValues.push_back({Utils::getCurrentDate(), totalValue});
    }
    
    // Calculate total portfolio value
    double getTotalValue() const {
        double total = 0.0;
        for (const auto& [symbol, asset] : assets) {
            total += asset->getCurrentValue();
        }
        return total;
    }
    
    // Calculate total return percentage
    double getTotalReturnPercentage() const {
        if (initialInvestment <= 0) return 0.0;
        return ((getTotalValue() - initialInvestment) / initialInvestment) * 100.0;
    }
    
    // Get portfolio composition as percentages
    std::map<std::string, double> getPortfolioComposition() const {
        std::map<std::string, double> composition;
        double totalValue = getTotalValue();
        
        if (totalValue <= 0) return composition;
        
        for (const auto& [symbol, asset] : assets) {
            composition[symbol] = (asset->getCurrentValue() / totalValue) * 100.0;
        }
        
        return composition;
    }
    
    // Rebalance portfolio based on risk analyzer recommendations
    void rebalancePortfolio() {
        auto recommendations = riskAnalyzer.recommendRebalancing(assets);
        
        if (recommendations.empty()) {
            std::cout << "Portfolio is well-balanced. No rebalancing needed." << std::endl;
            return;
        }
        
        std::cout << "\n========== REBALANCING PORTFOLIO ==========\n" << std::endl;
        
        double totalValue = getTotalValue();
        
        for (const auto& [symbol, percentageDiff] : recommendations) {
            double targetAmount = totalValue * (std::abs(percentageDiff) / 100.0);
            
            if (percentageDiff > 0) {
                // Need to buy more of this asset
                std::cout << "Recommendation: BUY " << Utils::formatCurrency(targetAmount) 
                          << " worth of " << symbol << " (increase by " 
                          << std::fixed << std::setprecision(1) << percentageDiff << "%)" << std::endl;
                
                if (assets.find(symbol) != assets.end()) {
                    assets[symbol]->buy(targetAmount);
                }
            } else {
                // Need to sell some of this asset
                std::cout << "Recommendation: SELL " << Utils::formatCurrency(targetAmount) 
                          << " worth of " << symbol << " (decrease by " 
                          << std::fixed << std::setprecision(1) << std::abs(percentageDiff) << "%)" << std::endl;
                
                if (assets.find(symbol) != assets.end()) {
                    double sellPercentage = std::abs(percentageDiff);
                    assets[symbol]->sell(sellPercentage);
                }
            }
        }
        
        lastRebalanceDate = Utils::getCurrentDate();
        recordPortfolioValue();
        std::cout << "\nRebalancing completed on " << lastRebalanceDate << std::endl;
    }
    
    // Get risk analyzer reference
    RiskAnalyzer& getRiskAnalyzer() {
        return riskAnalyzer;
    }
    
    // Get SIP manager reference
    SIPManager& getSIPManager() {
        return sipManager;
    }
    
    // Display portfolio summary
    void displayPortfolioSummary() const {
        std::cout << "\n========== PORTFOLIO SUMMARY ==========\n" << std::endl;
        
        double totalValue = getTotalValue();
        double totalReturn = getTotalReturnPercentage();
        
        std::cout << "Total Portfolio Value: " << Utils::formatCurrency(totalValue) << std::endl;
        std::cout << "Initial Investment: " << Utils::formatCurrency(initialInvestment) << std::endl;
        std::cout << "Total Return: " << std::fixed << std::setprecision(2) << totalReturn << "%" << std::endl;
        std::cout << "Gain/Loss: " << Utils::formatCurrency(totalValue - initialInvestment) << std::endl;
        
        if (!lastRebalanceDate.empty()) {
            std::cout << "Last Rebalanced: " << lastRebalanceDate << std::endl;
        }
        
        std::cout << "\n--- Asset Breakdown ---" << std::endl;
        auto composition = getPortfolioComposition();
        
        for (const auto& [symbol, asset] : assets) {
            std::cout << "\n" << symbol << ":" << std::endl;
            std::cout << "  Value: " << Utils::formatCurrency(asset->getCurrentValue()) << std::endl;
            std::cout << "  Allocation: " << std::fixed << std::setprecision(1) 
                      << composition.at(symbol) << "%" << std::endl;
            std::cout << "  Return: " << std::fixed << std::setprecision(2) 
                      << asset->getReturnPercentage() << "%" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Display detailed portfolio analysis
    void displayDetailedAnalysis() const {
        std::cout << "\n========== DETAILED PORTFOLIO ANALYSIS ==========\n" << std::endl;
        
        for (const auto& [symbol, asset] : assets) {
            asset->display();
        }
        
        // Portfolio-level metrics
        double portfolioVolatility = riskAnalyzer.calculatePortfolioVolatility(assets);
        double riskAdjustedReturn = riskAnalyzer.calculateRiskAdjustedReturn(assets);
        
        std::cout << "--- Portfolio Metrics ---" << std::endl;
        std::cout << "Portfolio Volatility: " << std::fixed << std::setprecision(2) 
                  << portfolioVolatility << "%" << std::endl;
        std::cout << "Risk-Adjusted Return: " << std::fixed << std::setprecision(2) 
                  << riskAdjustedReturn << std::endl;
        
        // Display ASCII pie chart
        auto composition = getPortfolioComposition();
        std::cout << "\n--- Portfolio Composition ---" << std::endl;
        std::cout << Utils::generateASCIIPieChart(composition) << std::endl;
    }
    
    // Get all assets
    const std::map<std::string, std::shared_ptr<Asset>>& getAssets() const {
        return assets;
    }
};

// Advisor Engine class for generating recommendations
class AdvisorEngine {
private:
    PortfolioManager& portfolioManager;
    MarketDataFetcher& dataFetcher;
    std::vector<std::string> recommendations;
    std::vector<std::string> alerts;

public:
    AdvisorEngine(PortfolioManager& pm, MarketDataFetcher& df) 
        : portfolioManager(pm), dataFetcher(df) {}
    
    // Analyze market conditions and generate recommendations
    void analyzeAndRecommend() {
        recommendations.clear();
        alerts.clear();
        
        // Get market data
        double vix = dataFetcher.getVIX();
        double btcPrice = dataFetcher.getPrice("BTC");
        
        // Analyze individual assets
        analyzeAssets();
        
        // Market condition analysis
        analyzeMarketConditions(vix);
        
        // Portfolio balance analysis
        analyzePortfolioBalance();
        
        // Risk analysis
        analyzeRiskMetrics();
        
        // Generate specific buy/sell/hold signals
        generateTradingSignals();
    }
    
    // Analyze individual assets
    void analyzeAssets() {
        const auto& assets = portfolioManager.getAssets();
        
        for (const auto& [symbol, asset] : assets) {
            double volatility = asset->getVolatility();
            double returnPercentage = asset->getReturnPercentage();
            
            // High volatility warning
            if (volatility > 25.0) {
                alerts.push_back("HIGH VOLATILITY ALERT: " + symbol + " showing " + 
                               std::to_string(static_cast<int>(volatility)) + "% volatility");
            }
            
            // Performance analysis
            if (returnPercentage > 20.0) {
                recommendations.push_back("PROFIT TAKING: Consider taking profits on " + symbol + 
                                        " (+" + std::to_string(static_cast<int>(returnPercentage)) + "%)");
            } else if (returnPercentage < -15.0) {
                recommendations.push_back("REVIEW POSITION: " + symbol + " is down " + 
                                        std::to_string(static_cast<int>(std::abs(returnPercentage))) + 
                                        "%. Consider averaging down or cutting losses");
            }
        }
    }
    
    // Analyze market conditions
    void analyzeMarketConditions(double vix) {
        if (vix > 30.0) {
            alerts.push_back("MARKET VOLATILITY HIGH: VIX at " + std::to_string(static_cast<int>(vix)) + 
                           ". Consider reducing risk exposure");
            recommendations.push_back("Increase allocation to defensive assets (Gold, USD)");
            recommendations.push_back("Reduce crypto and forex exposure temporarily");
        } else if (vix < 15.0) {
            recommendations.push_back("MARKET CALM: VIX low at " + std::to_string(static_cast<int>(vix)) + 
                                    ". Good time to increase risk exposure");
        }
        
        // Currency analysis
        double usdInr = dataFetcher.getPrice("USD/INR");
        if (usdInr > 80.0) {
            recommendations.push_back("USD/INR HIGH: Consider reducing USD exposure and increasing INR assets");
        }
        
        // Bitcoin analysis
        double btcPrice = dataFetcher.getPrice("BTC");
        if (btcPrice > 50000.0) {
            recommendations.push_back("BITCOIN OVERBOUGHT: Consider taking profits or reducing BTC allocation");
        } else if (btcPrice < 30000.0) {
            recommendations.push_back("BITCOIN OVERSOLD: Good opportunity to increase BTC allocation");
        }
    }
    
    // Analyze portfolio balance
    void analyzePortfolioBalance() {
        auto composition = portfolioManager.getPortfolioComposition();
        
        // Check for over-concentration
        for (const auto& [symbol, percentage] : composition) {
            if (percentage > 40.0) {
                alerts.push_back("CONCENTRATION RISK: " + symbol + " represents " + 
                               std::to_string(static_cast<int>(percentage)) + "% of portfolio");
                recommendations.push_back("Consider rebalancing to reduce " + symbol + " concentration");
            }
        }
        
        // Check if rebalancing is needed
        auto rebalanceRecommendations = portfolioManager.getRiskAnalyzer().recommendRebalancing(
            portfolioManager.getAssets());
        
        if (!rebalanceRecommendations.empty()) {
            recommendations.push_back("REBALANCING NEEDED: Portfolio allocation has drifted from target");
        }
    }
    
    // Analyze risk metrics
    void analyzeRiskMetrics() {
        double portfolioVolatility = portfolioManager.getRiskAnalyzer().calculatePortfolioVolatility(
            portfolioManager.getAssets());
        double riskAdjustedReturn = portfolioManager.getRiskAnalyzer().calculateRiskAdjustedReturn(
            portfolioManager.getAssets());
        
        if (portfolioVolatility > 20.0) {
            alerts.push_back("HIGH PORTFOLIO VOLATILITY: " + 
                           std::to_string(static_cast<int>(portfolioVolatility)) + "%");
            recommendations.push_back("Consider adding more stable assets to reduce overall volatility");
        }
        
        if (riskAdjustedReturn < 0.5) {
            recommendations.push_back("LOW RISK-ADJUSTED RETURN: Review asset allocation for better efficiency");
        }
    }
    
    // Generate specific trading signals
    void generateTradingSignals() {
        const auto& assets = portfolioManager.getAssets();
        
        for (const auto& [symbol, asset] : assets) {
            std::string signal = generateSignalForAsset(symbol, asset);
            if (!signal.empty()) {
                recommendations.push_back(signal);
            }
        }
    }
    
    // Generate signal for individual asset
    std::string generateSignalForAsset(const std::string& symbol, std::shared_ptr<Asset> asset) {
        double returnPercentage = asset->getReturnPercentage();
        double volatility = asset->getVolatility();
        
        // Simple signal logic
        if (symbol == "BTC") {
            if (returnPercentage > 15.0 && volatility > 20.0) {
                return "BTC SIGNAL: SELL - High gains with high volatility suggest profit-taking";
            } else if (returnPercentage < -10.0 && volatility < 15.0) {
                return "BTC SIGNAL: BUY - Oversold with stabilizing volatility";
            } else {
                return "BTC SIGNAL: HOLD - Wait for clearer trend";
            }
        } else if (symbol == "XAU/USD") {
            if (volatility < 5.0 && returnPercentage < 5.0) {
                return "GOLD SIGNAL: BUY - Stable and underperforming, good hedge opportunity";
            } else if (returnPercentage > 10.0) {
                return "GOLD SIGNAL: HOLD - Good performance, maintain position";
            }
        } else if (symbol.find('/') != std::string::npos) {
            // Forex signals
            if (volatility > 15.0) {
                return symbol + " SIGNAL: REDUCE - High forex volatility, reduce exposure";
            } else if (returnPercentage > 8.0) {
                return symbol + " SIGNAL: HOLD - Good forex performance, maintain position";
            }
        }
        
        return "";
    }
    
    // Display all recommendations and alerts
    void displayRecommendations() const {
        std::cout << "\n========== AI ADVISOR RECOMMENDATIONS ==========\n" << std::endl;
        
        if (!alerts.empty()) {
            std::cout << "🚨 ALERTS:" << std::endl;
            for (const auto& alert : alerts) {
                std::cout << "  • " << alert << std::endl;
            }
            std::cout << std::endl;
        }
        
        if (!recommendations.empty()) {
            std::cout << "💡 RECOMMENDATIONS:" << std::endl;
            for (const auto& rec : recommendations) {
                std::cout << "  • " << rec << std::endl;
            }
            std::cout << std::endl;
        }
        
        if (alerts.empty() && recommendations.empty()) {
            std::cout << "✅ No immediate actions required. Portfolio looks healthy!" << std::endl;
        }
    }
    
    // Get monthly portfolio report
    void generateMonthlyReport() const {
        std::cout << "\n========== MONTHLY PORTFOLIO REPORT ==========\n" << std::endl;
        std::cout << "Report Date: " << Utils::getCurrentDate() << std::endl;
        
        // Portfolio performance
        double totalValue = portfolioManager.getTotalValue();
        double totalReturn = portfolioManager.getTotalReturnPercentage();
        
        std::cout << "\n--- Performance Summary ---" << std::endl;
        std::cout << "Portfolio Value: " << Utils::formatCurrency(totalValue) << std::endl;
        std::cout << "Total Return: " << std::fixed << std::setprecision(2) << totalReturn << "%" << std::endl;
        
        // Monthly projections
        auto& sipManager = const_cast<PortfolioManager&>(portfolioManager).getSIPManager();
        double monthlyInvestment = sipManager.getMonthlyAmount();
        
        std::cout << "\n--- SIP Growth Projections ---" << std::endl;
        std::cout << "Monthly Investment: " << Utils::formatCurrency(monthlyInvestment) << std::endl;
        std::cout << "Projected Value (1 year): " << Utils::formatCurrency(
            sipManager.calculateProjectedGrowth(12, 10.0)) << std::endl;
        std::cout << "Projected Value (5 years): " << Utils::formatCurrency(
            sipManager.calculateProjectedGrowth(60, 10.0)) << std::endl;
        
        // Risk assessment
        std::cout << "\n--- Risk Assessment ---" << std::endl;
        double portfolioVolatility = const_cast<PortfolioManager&>(portfolioManager)
            .getRiskAnalyzer().calculatePortfolioVolatility(portfolioManager.getAssets());
        std::cout << "Portfolio Volatility: " << std::fixed << std::setprecision(2) 
                  << portfolioVolatility << "%" << std::endl;
        
        // Asset performance
        std::cout << "\n--- Top Performers ---" << std::endl;
        std::vector<std::pair<std::string, double>> assetReturns;
        
        for (const auto& [symbol, asset] : portfolioManager.getAssets()) {
            assetReturns.push_back({symbol, asset->getReturnPercentage()});
        }
        
        std::sort(assetReturns.begin(), assetReturns.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (size_t i = 0; i < std::min(size_t(3), assetReturns.size()); ++i) {
            std::cout << "  " << (i+1) << ". " << assetReturns[i].first << ": " 
                      << std::fixed << std::setprecision(2) << assetReturns[i].second << "%" << std::endl;
        }
        
        std::cout << std::endl;
    }
};

// CLI Interface class for user interaction
class CLIInterface {
private:
    UserProfile userProfile;
    std::unique_ptr<PortfolioManager> portfolioManager;
    std::unique_ptr<MarketDataFetcher> dataFetcher;
    std::unique_ptr<AdvisorEngine> advisorEngine;
    bool isInitialized;

public:
    CLIInterface() : isInitialized(false) {
        dataFetcher = std::make_unique<MarketDataFetcher>();
    }
    
    ~CLIInterface() {
        // Smart pointers will automatically clean up
        // But we should ensure curl cleanup is done
        if (dataFetcher) {
            dataFetcher.reset();
        }
    }
    
    // Main application loop
    void run() {
        displayWelcome();
        
        if (!setupUser()) {
            std::cout << "Setup failed. Exiting..." << std::endl;
            return;
        }
        
        mainMenu();
    }
    
    // Display welcome message
    void displayWelcome() const {
        std::cout << "\n";
        std::cout << "██████╗ ██╗   ██╗███╗   ██╗ █████╗ ███╗   ███╗██╗ ██████╗" << std::endl;
        std::cout << "██╔══██╗╚██╗ ██╔╝████╗  ██║██╔══██╗████╗ ████║██║██╔════╝" << std::endl;
        std::cout << "██║  ██║ ╚████╔╝ ██╔██╗ ██║███████║██╔████╔██║██║██║     " << std::endl;
        std::cout << "██║  ██║  ╚██╔╝  ██║╚██╗██║██╔══██║██║╚██╔╝██║██║██║     " << std::endl;
        std::cout << "██████╔╝   ██║   ██║ ╚████║██║  ██║██║ ╚═╝ ██║██║╚██████╗" << std::endl;
        std::cout << "╚═════╝    ╚═╝   ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝" << std::endl;
        std::cout << std::endl;
        std::cout << "     🤖 AI-POWERED PERSONAL FINANCIAL ADVISOR 🤖" << std::endl;
        std::cout << "           Advanced Portfolio Management System" << std::endl;
        std::cout << std::endl;
    }
    
    // Set up user profile and initialize portfolio
    bool setupUser() {
        userProfile.setup();
        userProfile.displayProfile();
        
        // Initialize portfolio manager
        portfolioManager = std::make_unique<PortfolioManager>(userProfile);
        portfolioManager->initializePortfolio(userProfile.getInvestmentCapital());
        
        // Initialize advisor engine
        advisorEngine = std::make_unique<AdvisorEngine>(*portfolioManager, *dataFetcher);
        
        isInitialized = true;
        
        std::cout << "✅ Portfolio initialized successfully!" << std::endl;
        std::cout << "💰 Initial allocation completed based on your risk profile." << std::endl;
        
        return true;
    }
    
    // Main menu system
    void mainMenu() {
        while (true) {
            displayMainMenu();
            
            int choice;
            std::cout << "Enter your choice: ";
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    viewPortfolioSummary();
                    break;
                case 2:
                    viewDetailedAnalysis();
                    break;
                case 3:
                    updateMarketData();
                    break;
                case 4:
                    getAIRecommendations();
                    break;
                case 5:
                    manageSIP();
                    break;
                case 6:
                    rebalancePortfolio();
                    break;
                case 7:
                    generateReport();
                    break;
                case 8:
                    adjustRiskProfile();
                    break;
                case 9:
                    simulateScenarios();
                    break;
                case 0:
                    std::cout << "\n👋 Thank you for using Dynamic AI Financial Advisor!" << std::endl;
                    std::cout << "💡 Remember: Invest wisely and stay diversified!" << std::endl;
                    return;
                default:
                    std::cout << "❌ Invalid choice. Please try again." << std::endl;
            }
            
            pauseAndClear();
        }
    }
    
    // Display main menu options
    void displayMainMenu() const {
        std::cout << "\n========== MAIN MENU ==========\n" << std::endl;
        std::cout << "1. 📊 View Portfolio Summary" << std::endl;
        std::cout << "2. 🔍 Detailed Portfolio Analysis" << std::endl;
        std::cout << "3. 📈 Update Market Data" << std::endl;
        std::cout << "4. 🤖 Get AI Recommendations" << std::endl;
        std::cout << "5. 💰 Manage SIP Investments" << std::endl;
        std::cout << "6. ⚖️  Rebalance Portfolio" << std::endl;
        std::cout << "7. 📋 Generate Monthly Report" << std::endl;
        std::cout << "8. 🎯 Adjust Risk Profile" << std::endl;
        std::cout << "9. 🔮 Simulate Scenarios" << std::endl;
        std::cout << "0. 🚪 Exit" << std::endl;
        std::cout << std::endl;
    }
    
    // View portfolio summary
    void viewPortfolioSummary() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        portfolioManager->displayPortfolioSummary();
    }
    
    // View detailed analysis
    void viewDetailedAnalysis() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        portfolioManager->displayDetailedAnalysis();
    }
    
    // Update market data
    void updateMarketData() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "🔄 Updating market data..." << std::endl;
        portfolioManager->updatePrices(false); // Use simulated data for demo
        std::cout << "✅ Market data updated successfully!" << std::endl;
    }
    
    // Get AI recommendations
    void getAIRecommendations() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "🤖 Analyzing portfolio and market conditions..." << std::endl;
        advisorEngine->analyzeAndRecommend();
        advisorEngine->displayRecommendations();
    }
    
    // SIP management
    void manageSIP() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "\n========== SIP MANAGEMENT ==========\n" << std::endl;
        std::cout << "1. View SIP Details" << std::endl;
        std::cout << "2. Execute SIP Investment" << std::endl;
        std::cout << "3. Modify SIP Amount" << std::endl;
        std::cout << "4. Change SIP Allocation" << std::endl;
        std::cout << "5. Toggle Auto-Invest" << std::endl;
        std::cout << "0. Back to Main Menu" << std::endl;
        
        int choice;
        std::cout << "Enter choice: ";
        std::cin >> choice;
        
        auto& sipManager = portfolioManager->getSIPManager();
        
        switch (choice) {
            case 1:
                sipManager.display();
                break;
            case 2:
                portfolioManager->executeSIPInvestment(true);
                std::cout << "✅ SIP investment executed!" << std::endl;
                break;
            case 3: {
                double newAmount;
                std::cout << "Enter new monthly SIP amount: $";
                std::cin >> newAmount;
                sipManager.setMonthlyAmount(newAmount);
                std::cout << "✅ SIP amount updated!" << std::endl;
                break;
            }
            case 4:
                std::cout << "📝 Current allocation modification not implemented in demo." << std::endl;
                std::cout << "💡 Use rebalancing feature to adjust overall allocation." << std::endl;
                break;
            case 5:
                sipManager.toggleAutoInvest();
                std::cout << "🔄 Auto-invest toggled to: " 
                          << (sipManager.getAutoInvestStatus() ? "ON" : "OFF") << std::endl;
                break;
        }
    }
    
    // Rebalance portfolio
    void rebalancePortfolio() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "⚖️ Analyzing portfolio balance..." << std::endl;
        portfolioManager->rebalancePortfolio();
    }
    
    // Generate monthly report
    void generateReport() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        advisorEngine->generateMonthlyReport();
    }
    
    // Adjust risk profile
    void adjustRiskProfile() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "\n========== RISK PROFILE ADJUSTMENT ==========\n" << std::endl;
        auto& riskAnalyzer = portfolioManager->getRiskAnalyzer();
        
        std::cout << "Current Risk Score: " << riskAnalyzer.getRiskScore() << "/100" << std::endl;
        std::cout << "Current Profile: " << riskAnalyzer.getRiskProfileStr() << std::endl;
        
        std::cout << "\nEnter new risk score (0-100): ";
        double newRiskScore;
        std::cin >> newRiskScore;
        
        riskAnalyzer.setRiskScore(newRiskScore);
        std::cout << "✅ Risk profile updated!" << std::endl;
        std::cout << "💡 Consider rebalancing portfolio to match new risk profile." << std::endl;
    }
    
    // Simulate scenarios
    void simulateScenarios() {
        if (!isInitialized) {
            std::cout << "❌ Portfolio not initialized!" << std::endl;
            return;
        }
        
        std::cout << "\n========== SCENARIO SIMULATION ==========\n" << std::endl;
        std::cout << "📊 Simulating market scenarios..." << std::endl;
        
        double currentValue = portfolioManager->getTotalValue();
        
        // Simulate different market scenarios
        std::cout << "\n--- Market Scenario Analysis ---" << std::endl;
        std::cout << "Current Portfolio Value: " << Utils::formatCurrency(currentValue) << std::endl;
        
        // Bull market scenario (+20% growth)
        double bullValue = currentValue * 1.20;
        std::cout << "🐂 Bull Market (+20%): " << Utils::formatCurrency(bullValue) << std::endl;
        
        // Bear market scenario (-30% decline)
        double bearValue = currentValue * 0.70;
        std::cout << "🐻 Bear Market (-30%): " << Utils::formatCurrency(bearValue) << std::endl;
        
        // Recession scenario (-40% decline)
        double recessionValue = currentValue * 0.60;
        std::cout << "📉 Recession (-40%): " << Utils::formatCurrency(recessionValue) << std::endl;
        
        // High inflation scenario
        std::cout << "\n--- Inflation Impact Analysis ---" << std::endl;
        double inflationRate = 8.0; // 8% inflation
        double realValue1Year = currentValue / std::pow(1 + inflationRate/100.0, 1);
        double realValue5Years = currentValue / std::pow(1 + inflationRate/100.0, 5);
        
        std::cout << "💸 Real Value (1 year, 8% inflation): " 
                  << Utils::formatCurrency(realValue1Year) << std::endl;
        std::cout << "💸 Real Value (5 years, 8% inflation): " 
                  << Utils::formatCurrency(realValue5Years) << std::endl;
        
        // SIP Growth Simulation
        std::cout << "\n--- SIP Growth Scenarios ---" << std::endl;
        auto& sipManager = portfolioManager->getSIPManager();
        double monthlyInvestment = sipManager.getMonthlyAmount();
        
        if (monthlyInvestment > 0) {
            double conservativeGrowth = sipManager.calculateProjectedGrowth(120, 8.0);  // 8% annual
            double moderateGrowth = sipManager.calculateProjectedGrowth(120, 12.0);     // 12% annual
            double aggressiveGrowth = sipManager.calculateProjectedGrowth(120, 15.0);   // 15% annual
            
            std::cout << "🟢 Conservative (8% annual, 10 years): " 
                      << Utils::formatCurrency(conservativeGrowth) << std::endl;
            std::cout << "🟡 Moderate (12% annual, 10 years): " 
                      << Utils::formatCurrency(moderateGrowth) << std::endl;
            std::cout << "🔴 Aggressive (15% annual, 10 years): " 
                      << Utils::formatCurrency(aggressiveGrowth) << std::endl;
        }
        
        std::cout << "\n💡 Scenarios help you prepare for different market conditions!" << std::endl;
    }
    
    // Pause and clear screen utility
    void pauseAndClear() {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
        
        // Clear screen (works on most terminals)
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }
};

// Main function
int main() {
    try {
        // Initialize the CLI interface and run the application
        CLIInterface app;
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Application error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown error occurred!" << std::endl;
        return 1;
    }
    
    return 0;
}